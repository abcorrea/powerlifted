import os

def get_elapsed_time():
    """
    Return the CPU time taken by the python process and its child
    processes.

    From Fast Downward source code.
    """
    if os.name == "nt":
        # The child time components of os.times() are 0 on Windows.
        raise NotImplementedError("cannot use get_elapsed_time() on Windows")
    return sum(os.times()[:4])


def find_domain_filename(task_filename):
    """
    Find domain filename for the given task using automatic naming rules.
    """
    dirname, basename = os.path.split(task_filename)

    domain_basenames = [
        "domain.pddl",
        basename[:3] + "-domain.pddl",
        "domain_" + basename,
        "domain-" + basename,
    ]

    for domain_basename in domain_basenames:
        domain_filename = os.path.join(dirname, domain_basename)
        if os.path.exists(domain_filename):
            return domain_filename

def silent_remove(f):
    try:
        os.remove(f)
    except OSError:
        pass


def remove_temporary_files(options):
    running_dir = os.getcwd()
    silent_remove(options.translator_file)
    # Harcoded files for now
    silent_remove('new-instance-file.pddl')
    silent_remove('new-domain-file.pddl')
