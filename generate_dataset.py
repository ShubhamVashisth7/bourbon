# Generate a .txt file with numbers from 1 to 10000, each on a new line
file_path = "example_dataset.txt"

with open(file_path, "w") as file:
    for number in range(0, 100000000):
        file.write(f"{number}\n")

print("done.")
