#!/usr/bin/env python3.11
import csv
from datetime import datetime,timedelta,timezone
import math

wxlogfile="/home/jakob/gw1000/wxlog.csv"
adid="TROSA"

def read_sensor_csv_native(file_path):
    data = []
    with open(file_path, mode='r', newline='', encoding='utf-8') as f:
        for row in csv.reader(f):
            if not row: 
                continue
            # Convert index 0 to a datetime object, convert indices 1+ to floats
            parsed_row = [datetime.fromisoformat(row[0])] + [float(x) for x in row[1:]]
            data.append(parsed_row)
    return data

def filter_last_x_minutes_native(data_list,x):
    if not data_list:
        return []
        
    # Find the latest timestamp from the first item of each row
    #latest_time = max(row[0] for row in data_list)
    
    
    # Define the minute threshold window
    #cutoff_time = latest_time - timedelta(minutes=x)
    cutoff_time = datetime.now(timezone.utc) - timedelta(minutes=x)
    
    # Filter and keep rows that fall within the last 2 minutes
    return [row for row in data_list if row[0] >= cutoff_time]

def calculate_wind_averages_native(filtered_list):
    if not filtered_list:
        return None, None
        
    # Extract columns (10th column = index 9, 11th column = index 10)
    directions = [float(row[9]) for row in filtered_list]
    speeds = [float(row[10]) for row in filtered_list]
    
    # 1. Wind Speed Average
    avg_speed = sum(speeds) / len(speeds)
    
    # 2. Correct Wind Direction Vector Average
    sum_sin = 0.0
    sum_cos = 0.0
    
    for deg in directions:
        rad = math.radians(deg)
        sum_sin += math.sin(rad)
        sum_cos += math.cos(rad)
        
    avg_dir_rad = math.atan2(sum_sin, sum_cos)
    avg_dir_deg = math.degrees(avg_dir_rad)
    avg_dir_deg = (avg_dir_deg + 360) % 360

    # --- Formatting Modifications ---
    
    # 1. Round direction to the nearest 10 (e.g., 254 -> 250, 256 -> 260)
    # Passing -1 to round() targets the tens place
    final_direction = int(round(avg_dir_deg, -1))
    
    ### Keep direction within 0-360 boundaries if it rounds up to 360
    #if final_direction == 360:
    #    final_direction = 0
    # METAR should N as 360
    if final_direction == 0:
        final_direction = 360
        
    # 2. Round speed UP to nearest whole number
    ceil_speed = math.ceil(avg_speed)
    
    # 3. Zero-pad speed to two digits (e.g., 7 -> "07", 12 -> "12")
    # The :02d format specifies a decimal integer, 2 characters wide, padded with zeros
    final_speed_str = f"{ceil_speed:02d}"

    return final_direction, final_speed_str
    
#    return avg_dir_deg, avg_speed

# start of METAR
metar=adid
now=datetime.utcnow()
metar += now.strftime(' %d%H%MZ AUTO')

wxdata=read_sensor_csv_native(wxlogfile)

recent_2min_data = filter_last_x_minutes_native(wxdata,2)
print("len: "+str(len(recent_2min_data)))
if(len(recent_2min_data) < 10):
    print(adid+now.strftime(' %d%H%MZ METAR U/S'))
    exit(1)
recent_10min_data = filter_last_x_minutes_native(wxdata,10)
recent_30min_data = filter_last_x_minutes_native(wxdata,30)

wind_speeds = [float(row[10]) for row in recent_10min_data]
wind_speeds_gust = [float(row[10]) for row in recent_10min_data]

max_speed = max(wind_speeds_gust)
min_speed = min(wind_speeds)

# rain
rain_rate=wxdata[-1][16]
rain_last_30 = [float(row[16]) for row in recent_30min_data]
rain_last_30_sum=sum(rain_last_30)

#test
print(f"Max: {max_speed}, Min: {min_speed}, diff: {max_speed-min_speed}")

print(f"Rain rate {rain_rate}")
print(f"Rain sum 30 {rain_last_30_sum}")

# wind data
avg_dir, avg_spd = calculate_wind_averages_native(recent_2min_data)
if(int(avg_spd) < 1):
    metar+=" 00000KT"
else:
    if(int(avg_spd) < 5):
        metar+=" VRB"+str(avg_spd)
    else:
        metar+=" "+str(avg_dir)+str(avg_spd)

if((max_speed-min_speed) > 10):
    metar+="G"+f"{math.ceil(max_speed):02d}"

metar+="KT"

# raining?
if(rain_rate > 0):
    metar+=" "
    if(rain_rate < 2.5):
        metar+="-"
    if(rain_rate > 10):
        metar+="+"
    metar+="RA"

# recent rain?
if(rain_last_30_sum > 0):
    if(rain_rate == 0):
        metar+=" RERA"    
    

# temp/dewpoint
metar+=" "

if (round(wxdata[-1][4]) < 0):
    metar+="M"

metar+=f"{abs(round(wxdata[-1][4])):02d}/"

if (round(wxdata[-1][6]) < 0):
    metar+="M"

metar+=f"{abs(round(wxdata[-1][6])):02d}"

# QHN
metar+=f" Q{math.floor(wxdata[-1][7])}"

# end
metar+="="

print(metar)
#print(recent_data)

