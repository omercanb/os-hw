import os
import subprocess
import time
import csv

file_prefix = "testfile"
k = 0
num_files = 100000
n_values = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
data_len_values = [200, 400, 600, 800, 1000]
output_csv = "experiment_results.csv"

data_dir = "experiment2"


def generate_files():
    if not os.path.exists(data_dir):
        os.makedirs(data_dir)

    string = "the quick brown fox jumps over the lazy dog "
    content = string * (1024 // len(string))

    for i in range(1, num_files + 1):
        if (i + 1) % 10000 == 0:
            print(f"On file {i}")
        with open(f"{data_dir}/{file_prefix}{i}", "w") as f:
            f.write(content)


def run_experiment(exec_name, args):
    start_time = time.time()
    try:
        # Use subprocess.run to execute the C program
        subprocess.run([f"./{exec_name}"] + list(map(str, args)),
                       check=True, capture_output=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running {exec_name}: {e}")
        return None
    return time.time() - start_time


def main():
    # generate_files()
    thread_experiments()
    # check_experiments()
    # thread_experiments()
    # k_experiments()
    # generate_files()
    return
    results = []

    print(f"{'N':<5} | {'Multiprocess (findlwp)':<25} | {
          'Multithreaded (findlwt)':<25}")
    print("-" * 65)

    with open(output_csv, "w", newline="") as f:
        writer = csv.DictWriter(
            f, fieldnames=["dataLen", "n", "processTime", "threadTime"])
        writer.writeheader()

    for data_len in data_len_values:
        for n in n_values:
            path = f"{data_dir}/{file_prefix}"
            p_time = run_experiment(
                "findlwp", [path, n, k, data_len, "out_p.txt"])

            t_time = run_experiment(
                "findlwt", [path, n, k, "out_t.txt"])
            assert (p_time)
            assert (t_time)

            results.append({"dataLen": data_len, "n": n,
                           "processTime": p_time, "threadTime": t_time})
            print(f"{data_len:<5} | {n:<5} | {
                p_time:24.4f}s | {t_time:24.4f}s")

            with open(output_csv, "a", newline="") as f:
                writer = csv.DictWriter(
                    f, fieldnames=["dataLen", "n", "processTime", "threadTime"])
                writer.writerow({
                    "dataLen": data_len,
                    "n": n,
                    "processTime": p_time,
                    "threadTime": t_time
                })


def k_experiments():
    # generate_files()
    results = []

    print(f"{'N':<5} | {'Multiprocess (findlwp)':<25} | {
          'Multithreaded (findlwt)':<25}")
    print("-" * 65)

    csv_name = "k_experiments_1.csv"

    with open(csv_name, "w", newline="") as f:
        writer = csv.DictWriter(
            f, fieldnames=["k", "time", "processTime", "threadTime"])
        writer.writeheader()

    data_len = 1000
    n = 10
    for k in range(1, 11):
        path = f"{data_dir}/{file_prefix}"
        time = run_experiment(
            "findlwcheck", [path, n, k, "out.txt"]
        )
        p_time = run_experiment(
            "findlwp", [path, n, k, data_len, "out_p.txt"])

        t_time = run_experiment(
            "findlwt", [path, n, k, "out_t.txt"])
        assert (time)
        assert (p_time)
        assert (t_time)

        results.append({"k": k, "time": time,
                       "processTime": p_time, "threadTime": t_time})
        print(f"{k=}{time=}{p_time=}{t_time=}")

        with open(csv_name, "a", newline="") as f:
            writer = csv.DictWriter(
                f, fieldnames=["k", "time", "processTime", "threadTime"])
            writer.writerow({
                "k": k,
                "time": time,
                "processTime": p_time,
                "threadTime": t_time
            })


def check_experiments():
    results = []
    csv_name = "single_thread_experiment1.csv"

    print(f"{'N':<5} | {'Single Threaded (findlwcheck)':<25}")

    with open(csv_name, "w", newline="") as f:
        writer = csv.DictWriter(
            f, fieldnames=["dataLen", "n", "threadTime"])
        writer.writeheader()

    for n in n_values:
        path = f"{data_dir}/{file_prefix}"
        t_time = run_experiment(
            "findlwcheck", [path, n, k, "out.txt"])
        assert (t_time)

        results.append({"n": n,
                       "time": t_time})
        print(f"| {n:<5} | {t_time: 24.4f}s")

        with open(csv_name, "a", newline="") as f:
            writer = csv.DictWriter(
                f, fieldnames=["dataLen", "n", "time"])
            writer.writerow({
                "n": n,
                "time": t_time,
            })


def thread_experiments():
    results = []
    csv_name = "thread_experiment_3.csv"

    print(f"{'N':<5} | {'Multithreaded (findlwt)':<25}")

    with open(csv_name, "w", newline="") as f:
        writer = csv.DictWriter(
            f, fieldnames=["n", "time", "threadTime"])
        writer.writeheader()

    # for n in n_values:
    for n in range(1, num_files + 1, 1000):
        path = f"{data_dir}/{file_prefix}"
        time = run_experiment(
            "findlwcheck", [path, n, k, "out.txt"])
        t_time = run_experiment(
            "findlwt", [path, n, k, "out_t.txt"])
        assert (t_time)

        results.append({"n": n,
                        "time": time,
                       "threadTime": t_time})
        print(f"| {n:<5} | {time:24.4f}s | {t_time: 24.4f}s")

        with open(csv_name, "a", newline="") as f:
            writer = csv.DictWriter(
                f, fieldnames=["n", "time", "threadTime"])
            writer.writerow({
                "n": n,
                "time": time,
                "threadTime": t_time
            })


def process_experiments():
    results = []
    csv_name = "process_experiment_1.csv"

    print(f"{'N':<5} | {'Multiprocess (findlwp)':<25} | {
          'Multithreaded (findlwt)':<25}")
    print("-" * 65)

    with open(csv_name, "w", newline="") as f:
        writer = csv.DictWriter(
            f, fieldnames=["dataLen", "n", "processTime"])
        writer.writeheader()

    for data_len in data_len_values:
        for n in n_values:
            path = f"{data_dir}/{file_prefix}"
            p_time = run_experiment(
                "findlwp", [path, n, k, data_len, "out_p.txt"])

            assert (p_time)

            results.append({"dataLen": data_len, "n": n,
                           "processTime": p_time})
            print(f"{data_len:<5} | {n:<5} | {
                p_time:24.4f}s")

            with open(csv_name, "a", newline="") as f:
                writer = csv.DictWriter(
                    f, fieldnames=["dataLen", "n", "processTime"])
                writer.writerow({
                    "dataLen": data_len,
                    "n": n,
                    "processTime": p_time,
                })


if __name__ == "__main__":
    main()
