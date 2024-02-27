#!/bin/bash

# Define the sets of values for each parameter
DNs=(250)

# Define the IP addresses of the remote devices
remote_ips=("10.100.188.118" "10.100.190.34" "10.100.178.31" "10.100.190.25" "10.100.177.14")
#remote_ips=("10.100.188.118")

# Iterate over each combination of parameters
for DN in "${DNs[@]}"
do
    Delay=20

    # Run the Python script with the current set of parameters
    echo "$(date '+%Y-%m-%d %H:%M:%S') New logging started for DN=$DN, Delay=$Delay"
    for ip1 in "${remote_ips[@]}"
        do
        ssh pi@$ip1 "python3 ~/ACK_reliability_test/bash_setting_logger_DLcause.py $DN $Delay" &
    done
    # python3 ./bash_setting_logger.py $RT_num $DN $Trace_num $Delay

    wait

    echo "$(date '+%Y-%m-%d %H:%M:%S') Record done, No processing required."
done

