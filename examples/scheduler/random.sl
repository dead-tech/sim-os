schedule_policy :: FCFS
threads_count :: 9
max_processes :: 500
max_events_per_process :: 30
max_single_event_duration :: 15
max_arrival_time :: 30

for 0..300 {
    spawn_random_process()
}
