
def discriminate_org_synt(run):
    if run['domain'] != 'organic-synthesis-opt18-strips':
        run['domain'] = 'other'
    return run

def domain_as_category(run1, run2):
     # run2['domain'] has the same value, because we always
     # compare two runs of the same problem.
     return run1['domain']
