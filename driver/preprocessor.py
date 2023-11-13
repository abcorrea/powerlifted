import os
import subprocess


def preprocess_cpddl(options):
    CPDDL = os.environ.get('CPDDL_BIN')
    if CPDDL is None:
        raise("ERROR: CPDDL_BIN not set. It should point to the bin folder of your local cpddl installation")

    domain = options.domain
    instance = options.instance

    # Name of the new domain and instance files:
    new_domain = os.path.join(os.getcwd(), 'new-domain-file.pddl')
    new_instance = os.path.join(os.getcwd(), 'new-instance-file.pddl')

    cpddl = subprocess.Popen([os.path.join(CPDDL, 'pddl-pddl'),
                                   '--pddl-domain-out', new_domain,
                                   '--pddl-problem-out', new_instance,
                                   domain, instance,
                                   '--pddl-ce'],
                             stderr=subprocess.STDOUT)

    cpddl.communicate()
    if cpddl.returncode != 0:
        raise RuntimeError("Error during CPDDL preprocessing.")

    options.domain = new_domain
    options.instance = new_instance
