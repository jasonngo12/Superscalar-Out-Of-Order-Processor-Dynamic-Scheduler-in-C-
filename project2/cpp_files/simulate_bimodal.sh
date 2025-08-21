#!/bin/bash

# Create or clear the output CSV file
output_file="bimodal_misprediction_rate_perl.csv"
echo "m,misprediction_rate" > $output_file

# Run the simulation for values of m from 7 to 20
for m in {7..20}; do
    # Run the simulation command and capture the output
    result=$(./sim bimodal $m perl_trace.txt)

    # Extract the misprediction rate from the simulation output
    misprediction_rate=$(echo "$result" | grep "misprediction rate" | awk '{print $3}')

    # Append the value of m and misprediction rate to the CSV file
    echo "$m,$misprediction_rate" >> $output_file
done

echo "Data saved to $output_file. You can now import this CSV into Excel."
