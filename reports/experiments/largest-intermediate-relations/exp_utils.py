
def discriminate_acyclic(run):
    acyclic = ['agricola-opt18-strips',
    'barman-opt11-strips',
    'barman-opt14-strips',
    'caldera-split-opt18-adl',
    'data-network-opt18-strips',
    'elevators-opt08-strips',
    'elevators-opt11-strips',
    'freecell',
    'hiking-opt14-strips',
    'nomystery-opt11-strips',
    'organic-synthesis-opt18-strips',
    'parcprinter-08-strips',
    'parcprinter-opt11-strips',
    'pipesworld-notankage',
    'pipesworld-tankage',
    'rovers',
    'satellite',
    'settlers-opt18-adl',
    'spider-opt18-strips',
    'termes-opt18-strips',
    'tetris-opt14-strips',
    'tidybot-opt11-strips',
    'tidybot-opt14-strips',
    'tpp',
    'genome-edit-distance',
    'genome-edit-distance-split',
    'organic-synthesis-alkene',
    'organic-synthesis-MIT',
    'organic-synthesis-original',
    'pipesworld-tankage-nosplit']
    if run['domain'] in acyclic:
        run['domain'] = 'acyclic'
    else:
        run['domain'] = 'other'
    return run
    
def discriminate_org_synt(run):
    if run['domain'] != 'organic-synthesis-opt18-strips':
        run['domain'] = 'other'
    return run

def domain_as_category(run1, run2):
     # run2['domain'] has the same value, because we always
     # compare two runs of the same problem.
     return run1['domain']
