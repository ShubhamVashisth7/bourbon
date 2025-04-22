#include <cassert>
#include <chrono>
#include <iostream>
#include "leveldb/db.h"
#include "leveldb/comparator.h"
#include "util.h"
#include "stats.h"
#include "learned_index.h"
#include <cstring>
#include "cxxopts.hpp"
#include <unistd.h>
#include <fstream>
#include "../db/version_set.h"
#include <cmath>
#include <random>
#include <thread>

using namespace leveldb;
using namespace adgMod;
using std::string;
using std::cout;
using std::endl;
using std::to_string;
using std::vector;
using std::map;
using std::ifstream;

int mix_base = 20;

enum LoadType {
    Ordered = 0,
    Reversed = 1,
    ReversedChunk = 2,
    Random = 3,
    RandomChunk = 4
};

uint64_t GenerateRandomIndex(uint64_t max_index) {
    static std::default_random_engine generator(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> distribution(0, max_index - 1);
    return distribution(generator);
}

int main(int argc, char *argv[]) {
    int rc;
    int num_operations, num_iteration, num_mix;
    float test_num_segments_base;
    float num_pair_step;
    string db_location, profiler_out, input_filename, distribution_filename, ycsb_filename;
    bool print_single_timing, print_file_info, evict, unlimit_fd, use_distribution = false, pause, use_ycsb = false;
    bool change_level_load, change_file_load, change_level_learning, change_file_learning;
    int load_type, insert_bound, num_threads, seed;
    string db_location_copy;

    cxxopts::Options commandline_options("leveldb read test", "Testing leveldb read performance.");
    commandline_options.add_options()
            ("n,get_number", "number of operations", cxxopts::value<int>(num_operations)->default_value("1000"))
            ("s,step", "the step of the loop of the size of db", cxxopts::value<float>(num_pair_step)->default_value("1"))
            ("i,iteration", "the number of iterations of a same size", cxxopts::value<int>(num_iteration)->default_value("1"))
            ("m,modification", "if set, run our modified version", cxxopts::value<int>(adgMod::MOD)->default_value("0"))
            ("h,help", "print help message", cxxopts::value<bool>()->default_value("false"))
            ("d,directory", "the directory of db", cxxopts::value<string>(db_location)->default_value("/mnt/tmp/"))
            ("k,key_size", "the size of key", cxxopts::value<int>(adgMod::key_size)->default_value("8"))
            ("v,value_size", "the size of value", cxxopts::value<int>(adgMod::value_size)->default_value("8"))
            ("single_timing", "print the time of every single get", cxxopts::value<bool>(print_single_timing)->default_value("false"))
            ("file_info", "print the file structure info", cxxopts::value<bool>(print_file_info)->default_value("false"))
            ("test_num_segments", "test: number of segments per level", cxxopts::value<float>(test_num_segments_base)->default_value("1"))
            ("string_mode", "test: use string or int in model", cxxopts::value<bool>(adgMod::string_mode)->default_value("false"))
            ("e,model_error", "error in modesl", cxxopts::value<uint32_t>(adgMod::model_error)->default_value("8"))
            ("f,input_file", "the filename of input file", cxxopts::value<string>(input_filename)->default_value(""))
            ("multiple", "test: use larger keys", cxxopts::value<uint64_t>(adgMod::key_multiple)->default_value("1"))
            ("w,write", "writedb", cxxopts::value<bool>(fresh_write)->default_value("false"))
            ("c,uncache", "evict cache", cxxopts::value<bool>(evict)->default_value("false"))
            ("u,unlimit_fd", "unlimit fd", cxxopts::value<bool>(unlimit_fd)->default_value("false"))
            ("x,dummy", "dummy option")
            ("l,load_type", "load type", cxxopts::value<int>(load_type)->default_value("0"))
            ("filter", "use filter", cxxopts::value<bool>(adgMod::use_filter)->default_value("false"))
            ("mix", "mix read and write", cxxopts::value<int>(num_mix)->default_value("0"))
            ("distribution", "operation distribution", cxxopts::value<string>(distribution_filename)->default_value(""))
            ("change_level_load", "load level model", cxxopts::value<bool>(change_level_load)->default_value("false"))
            ("change_file_load", "enable level learning", cxxopts::value<bool>(change_file_load)->default_value("false"))
            ("change_level_learning", "load file model", cxxopts::value<bool>(change_level_learning)->default_value("false"))
            ("change_file_learning", "enable file learning", cxxopts::value<bool>(change_file_learning)->default_value("false"))
            ("p,pause", "pause between operation", cxxopts::value<bool>(pause)->default_value("false"))
            ("policy", "learn policy", cxxopts::value<int>(adgMod::policy)->default_value("0"))
            ("YCSB", "use YCSB trace", cxxopts::value<string>(ycsb_filename)->default_value(""))
            ("insert", "insert new value", cxxopts::value<int>(insert_bound)->default_value("0"))
            ("t,threads", "# threads", cxxopts::value<int>(num_threads)->default_value("1"))
            ("seed", "seed", cxxopts::value<int>(seed)->default_value("62"));
    auto result = commandline_options.parse(argc, argv);
    if (result.count("help")) {
        printf("%s", commandline_options.help().c_str());
        exit(0);
    }
    
    srand(seed);
    std::default_random_engine e1(0), e2(255), e3(0);
    db_location_copy = db_location;
    adgMod::fd_limit = unlimit_fd ? 1024 * 1024 : 1024;
    adgMod::restart_read = true;
    adgMod::level_learning_enabled ^= change_level_learning;
    adgMod::file_learning_enabled ^= change_file_learning;
    adgMod::load_level_model ^= change_level_load;
    adgMod::load_file_model ^= change_file_load;

    vector<string> keys;
    vector<uint64_t> distribution;
    vector<int> ycsb_is_write;

    if (!input_filename.empty()) {        
        uint64_t total_keys, *data;
        std::ifstream is(input_filename, std::ios::binary | std::ios::in);
        cout << "reading " << input_filename << std::endl;
        if (!is.is_open()) {
            std::cout << input_filename << " does not exists" << std::endl;
            exit(0);
        }
        is.read(reinterpret_cast<char*>(&total_keys), sizeof(uint64_t));
        data = new uint64_t[total_keys];
        is.read(reinterpret_cast<char*>(data), total_keys*sizeof(uint64_t));
        is.close();
        for (int i = 0; i < total_keys; ++i) 
            keys.push_back(generate_key(to_string(data[i])));
        adgMod::key_size = (int) keys.front().size();
    } else {
        std::uniform_int_distribution<uint64_t> udist_key(0, 999999999999999);
        for (int i = 0; i < 10000000; ++i) {
            keys.push_back(generate_key(to_string(udist_key(e2))));
        }
    }
    // keys.resize(num_operations);
    cout << "sample keys: ";
    for (int i = 0; i < 5; ++i)
        cout << keys[i] << " ";
    cout << endl;

    if (!distribution_filename.empty()) {
        use_distribution = true;
        ifstream input(distribution_filename);
        uint64_t index;
        while (input >> index) {
            distribution.push_back(index);
        }
    }

    if (!ycsb_filename.empty()) {
        use_ycsb = true;
        use_distribution = true;
        ifstream input(ycsb_filename);
        uint64_t index;
        int is_write;
        while (input >> is_write >> index) {
            distribution.push_back(index);
            ycsb_is_write.push_back(is_write);
        }
    }
    bool copy_out = num_mix != 0 || use_ycsb;

    adgMod::Stats* instance = adgMod::Stats::GetInstance();
    vector<vector<size_t>> times(20);
    string values(1024 * 1024, '0');

    if (copy_out) {
        rc = system("sync; echo 3 | sudo tee /proc/sys/vm/drop_caches");
    }

    if (num_mix > 1000) {
        mix_base = 1000;
        num_mix -= 1000;
    }
    
    for (size_t iteration = 0; iteration < num_iteration; ++iteration) {
        db_location = db_location_copy;
        std::uniform_int_distribution<uint64_t > uniform_dist_file(0, (uint64_t) keys.size() - 1);
        std::uniform_int_distribution<uint64_t > uniform_dist_file2(0, (uint64_t) keys.size() - 1);
        std::uniform_int_distribution<uint64_t > uniform_dist_value(0, (uint64_t) values.size() - adgMod::value_size - 1);

        DB* db;
        Options options;
        ReadOptions& read_options = adgMod::read_options;
        WriteOptions& write_options = adgMod::write_options;
        Status status;

        options.create_if_missing = true;
        write_options.sync = true;
        instance->ResetAll();

        if (fresh_write && iteration == 0) {
            // Load DB
            // clear existing directory, clear page cache, trim SSD
            string command = "rm -rf " + db_location;
            rc = system(command.c_str());
            status = DB::Open(options, db_location, &db);
            assert(status.ok() && "Open Error");
            // different load order
            int cut_size = std::max(1, (int)(num_operations / 100000)); 
            std::vector<std::pair<int, int>> chunks;
            switch (load_type) {
                case Ordered: {
                    for (int cut = 0; cut < cut_size; ++cut) {
                        chunks.emplace_back(num_operations * cut / cut_size, num_operations * (cut + 1) / cut_size);
                    }
                    break;
                }
                case ReversedChunk: {
                    for (int cut = cut_size - 1; cut >= 0; --cut) {
                        chunks.emplace_back(num_operations * cut / cut_size, num_operations * (cut + 1) / cut_size);
                    }
                    break;
                }
                case Random: {
                    std::random_shuffle(keys.begin(), keys.end());
                    for (int cut = 0; cut < cut_size; ++cut) {
                        chunks.emplace_back(num_operations * cut / cut_size, num_operations * (cut + 1) / cut_size);
                    }
                    break;
                }
                case RandomChunk: {
                    for (int cut = 0; cut < cut_size; ++cut) {
                        chunks.emplace_back(num_operations * cut / cut_size, num_operations * (cut + 1) / cut_size);
                    }
                    std::random_shuffle(chunks.begin(), chunks.end());
                    break;
                }
                default: assert(false && "Unsupported load type.");
            }

            // perform load
            std::cout << "inserting " << num_operations << " keys\n";
            for (int cut = 0; cut < chunks.size(); ++cut) {
                if ((cut+1) % 50 == 0)
                    cout << "chunk inserted: " << cut+1 << "/" << cut_size << endl;
                for (int i = chunks[cut].first; i < chunks[cut].second; ++i) {
                    // cout << "inserting: " << keys[i] << endl;
                    status = db->Put(write_options, keys[i], {values.data() + uniform_dist_value(e2), (uint64_t) adgMod::value_size});
                    assert(status.ok() && "File Put Error");
                }
            }
            adgMod::db->vlog->Sync();

            // do offline leraning
            if (print_file_info && iteration == 0) db->PrintFileInfo();
            adgMod::db->WaitForBackground();
            // delete db;
            // status = DB::Open(options, db_location, &db);
            // adgMod::db->WaitForBackground();
            if (adgMod::MOD == 6 || adgMod::MOD == 7) {
                Version* current = adgMod::db->versions_->current();
                // level learning
                for (int i = 1; i < config::kNumLevels; ++i) {
                    LearnedIndexData::Learn(new VersionAndSelf{current, adgMod::db->version_count, current->learned_index_data_[i].get(), i});
                }
                // file learning
                current->FileLearn();
            }
            adgMod::db->WaitForBackground();
            fresh_write = false;
        }

        // for mix workloads, copy out the whole DB to preserve the original one
        if (copy_out) {
            string db_location_mix = db_location + "_mix";
            string remove_command = "rm -rf " + db_location_mix;
            string copy_command = "cp -r " + db_location + " " + db_location_mix;

            rc = system(remove_command.c_str());
            rc = system(copy_command.c_str());
            db_location = db_location_mix;
        }

        if (evict) rc = system("sync; echo 3 | sudo tee /proc/sys/vm/drop_caches");
        (void) rc;

        adgMod::db->WaitForBackground();

        uint64_t last_read = 0, last_write = 0;
        int last_level = 0, last_file = 0, last_baseline = 0, last_succeeded = 0, last_false = 0, last_compaction = 0, last_learn = 0;
        std::vector<uint64_t> detailed_times;
        bool start_new_event = true;

        uint64_t write_i = 0;
        // std::shuffle(keys.begin(), keys.end(), std::default_random_engine(seed));
        cout << "running " << num_operations << " operations with " << num_threads << " threads" << endl;
        int ops_per_thread = num_operations / num_threads; 
        cout << "ops_per_thread: " << ops_per_thread << endl;
       
        std::mutex cout_mutex;
        auto start = std::chrono::high_resolution_clock::now();
        int remaining_ops = num_operations % num_threads;
        int keys_per_thread = num_operations / num_threads;
        int remaining_keys = num_operations % num_threads;
        std::vector<std::thread> threads;
        int keys_start = 0;
        for (int t = 0; t < num_threads; ++t) {
            int chunk_keys = keys_per_thread + (t < remaining_keys ? 1 : 0);
            int keys_end = keys_start + chunk_keys;
            int thread_ops = ops_per_thread + (t < remaining_ops ? 1 : 0);
            threads.emplace_back([&, keys_start, keys_end, thread_ops]() {
                std::random_device rd;
                std::mt19937_64 generator(rd());
                std::uniform_int_distribution<uint64_t> dist(keys_start, keys_end - 1);
                for (int i = 0; i < thread_ops; ++i) {
                    bool write = use_ycsb ? ycsb_is_write[i] > 0 : (i % mix_base) < num_mix;
                    uint64_t random_key = dist(generator);
                    if (write) {
                        status = db->Put(write_options, keys[keys.size()-1], {values.data() + uniform_dist_value(e2), (uint64_t) adgMod::value_size});
                        assert(status.ok() && "File Put Error");
                    }
                    else {
                        string value;
                        ReadOptions local_read_options = read_options;
                        Status local_status = db->Get(local_read_options, keys[random_key], &value);
                        if (!local_status.ok() || value.size() != adgMod::value_size) {
                            // std::lock_guard<std::mutex> lock(cout_mutex);
                            cout << keys[random_key] << " absent or corrupted" << endl;
                        }
                    }
                }
            });
            keys_start = keys_end;
        }
        for (auto& thread : threads) {
            thread.join();
        }    
        auto end = std::chrono::high_resolution_clock::now();  
        adgMod::db->WaitForBackground();
        delete db;

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double seconds = duration.count() / 1000000;
        double throughput = num_operations / seconds;
        
        cout << "Threads: " << num_threads 
                  << ", Operations: " << num_operations
                  << ", Duration: " << seconds << " seconds"
                  << ", Throughput: " << throughput/1e6 << " Mops/s" 
                  << std::endl;
    }
}