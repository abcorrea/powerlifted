
from .limits import round_time_limit
from .utils import get_elapsed_time, remove_temporary_files
from .single_search_runner import run_single_search

def compute_run_time(timeout, relative_times, pos):
    '''
    From Fast Downward source code. Some parts were changed.
    '''

    remaining_time = timeout - get_elapsed_time()
    print("remaining time: {}".format(remaining_time))
    relative_time = relative_times[pos]
    remaining_relative_time = sum(t for t in relative_times[pos:])
    print("config {}: relative time {} seconds, remaining sum of relative times {}".format(
          pos, relative_time, remaining_relative_time))
    time_given = round_time_limit(remaining_time * relative_time / remaining_relative_time)
    print("run time given: %d seconds" % time_given)
    return round_time_limit(remaining_time * relative_time / remaining_relative_time)


def run(build_dir, options, extra):
    has_found_plan = False
    relative_times = []

    timeout = get_elapsed_time() + options.time_limit

    for it in options.iteration:
        # We first collect only the times
        _, _, _, time = it.split(',')
        relative_times.append(int(time))

    for count, it in enumerate(options.iteration):
        search, evaluator, generator, relative_time_str = it.split(',')
        relative_time = int(relative_time_str)
        print(f"Next iteration: {search}, {evaluator}, {generator}, {relative_time}")

        assert relative_time == relative_times[count]

        if count == len(options.iteration) - 1:
            print("Last iteration can use all remaining time.")
            relative_times[-1] = options.time_limit = get_elapsed_time()
            run_time = compute_run_time(timeout, relative_times, count)
        else:
            run_time = compute_run_time(timeout, relative_times, count)

        plan_name = ".".join([options.plan_file,str(count+1)])
        code = run_single_search(build_dir,
                                 run_time,
                                 options.translator_file,
                                 search,
                                 evaluator,
                                 generator,
                                 options.state,
                                 str(options.seed),
                                 plan_name,
                                 extra)

        # If we found a plan, check if we need to validate it
        # and then quit iterations
        if code == 0:
            has_found_a_plan = True
            if options.validate:
                validate(options.domain, options.instance, plan_name)
            if options.stop_after_first_plan:
                remove_temporary_files(options)
                return code

    remove_temporary_files(options)
    if has_found_plan:
        return 0
    else:
        return -1
