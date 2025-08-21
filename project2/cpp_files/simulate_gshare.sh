#!/bin/bash

# Output file for storing results
output_file="gshare_results.csv"
trace_file="gcc_trace.txt"

# Create or clear the output file
echo "m,n,misprediction_rate" > "$output_file"

# Loop over each m value from 7 to 20
for m in {7..20}; do
    # Loop over each n value from 0 to m
    for ((n=0; n<=m; n++)); do
        # Run the simulation command and capture the misprediction rate
        result=$(./sim gshare "$m" "$n" "$trace_file")
        
        # Extract the misprediction rate from the output
        misprediction_rate=$(echo "$result" | grep "misprediction rate" | awk '{print $3}')

        # Append the result to the output CSV file
        echo "$m,$n,$misprediction_rate" >> "$output_file"
        
        # Optional: display progress in the terminal
        echo "Completed: m=$m, n=$n, misprediction rate=$misprediction_rate"
    done
done

echo "Simulation completed. Results saved in $output_file."

