import os
import subprocess

def run_single_search(build_dir, time, translator_file, search, evaluator, generator, seed, plan_file, extra):
        cmd = [os.path.join(build_dir, 'search', 'search'),
               '-f', translator_file,
               '-s', search,
               '-e', evaluator,
               '-g', generator,
               '--seed', seed] + \
               ['--plan-file', plan_file] +\
               extra
        print(f'Executing "{" ".join(cmd)}"')
        try:
            code = subprocess.call(cmd, timeout=time)
            if code == 0:
                print("Iteration finished correctly.")
            else:
                print(f"Iteration finished with error code {code}.")
        except subprocess.TimeoutExpired as e:
            print(f"Iteration ran out of time: {e}")
            return -1
        except:
            print(f"Iteration finished with unknown error.")
        return code
