import json
import os, subprocess
import numpy as np
import pandas as pd

def genArgs(feats):
    arguments = './run_ctr ../fileDumps '+ feats
    return arguments


def getStat(args):
    res = str(subprocess.check_output(args,shell=True))
    print(res)
    timeStart = int(res.find("Time:")) + 5
    timeEnd = int(res.find("milliseconds"))
    probStart = int(res.find("Probability:")) + len("Probability:")
    probEnd = int(res.find("prod"))
    return float(res[timeStart: timeEnd]), float(res[probStart: probEnd])


def processOneRow(row):
    row = list(map(str, row))
    orderRow = row[:13] + row[14:] + [row[13]]
    strFeat = '"' + '" "'.join(orderRow) + '"'
    return strFeat

def main():
    df = pd.read_csv("../data/testingData.csv").sample(frac = 0.02, replace = False)
    df['combined'] = df.values.tolist()
    overAllTime = []
    overAllProb = []
    for row in df['combined']:
        procRow = processOneRow(row[:-1])
        args = genArgs(procRow)
        op = [getStat(args) for x in range(1)]
        overAllTime.append(np.mean([oneVal[0] for oneVal in op]))
        overAllProb.append(np.mean([oneVal[1] for oneVal in op]))
    print(np.mean(overAllTime), np.mean(overAllProb))

# ["82755" "1736554" "1935574" "589" ":pocoyo alphabet free" "0" "unknown" "320x50" "android" "wifi" "app" "p" "I" "www.xad.com" "4" "samsung" "galaxy j7" "2017" "spectrum" "00-03,7319" "us" "2019-07-18" "89"]
# ./run_ctr ../fileDumps "257375" "2677325" "4332535" "241" "color_bump_3d_android_color_bump_3d_android_320x50-t2_android_xxlarge_320x50_iab1:color_bump_3d_android_color_bump_3d_android_320x50-t2_android_xxlarge_320x50_iab1" "0" "m" "320x50" "android" "unknown" "app" "p" "I" "unknown" "4" "samsung" "galaxy s8" "2017" "at&t internet services" "08-11" "0" "us" "2019-07-19" "89"