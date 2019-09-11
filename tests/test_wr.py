import json
import os, subprocess
import random
from mars.feature_extraction import FeatureComposer, FeatureEngine
from mars.feature_extraction.feature import load_json
from mars.models.cc import ChainModel as ccChainModel
import pandas as pd


def randNum(cname):
    if cname == 'hour':
        return str(random.randint(0,23))
    elif cname == 'sl_adjusted_confidence':
        return str(random.randint(89,95))
    elif cname == 'weekday':
        return str(random.randint(1,7))

def dataGen(js):
    res = []
    feats = js['features']
    for f in feats:
        if f['type']=='OneHot':
            tmp = ' '
            while ' ' in tmp:
                tmp = random.sample(f['args']['values'],1)[0]
            res.append(tmp)
        else:
            res.append(randNum(f['args']['column']))
    return res

def genArgs(js,feats):
    args = './run_winrate tests/data/ '+' '.join(feats)
    # print(args)
    return args

def getStat(js,feats):
    res = str(subprocess.check_output(genArgs(js,feats),shell=True))
    p1 = res.find('win rate prediction')
    p2 = res.find('delivery rate prediction')
    p3 = res.find('final prediction')
    p4 = res.find('Prediction Time')
    p5 = res.find('milliseconds')
    stats = []
    num = float(res[(p1+21):(p2-5)])
    if num > 1:
        num = float(res[(p1+21):(p2-4)])
    stats.append(num)
    num = float(res[(p2+26):(p3-5)])
    if num>1:
        num = float(res[(p2+26):(p3-4)])
    stats.append(num)
    stats.append(float(res[(p4+17):(p5-1)]))
    return stats

def main():
    os.chdir('..')
    with open('tests/data/model_config1.json') as jfile:
        js = json.load(jfile)

    feat_obj = [load_json(v) for v in js['features']]
    columns = [f.columns[0] for f in feat_obj]
    engine = FeatureEngine(columns)
    cid = engine.add_composer(js['features'])
    chain1 = ccChainModel.from_avro('tests/data/wr_model_object.data')
    chain2 = ccChainModel.from_avro('tests/data/delivery_model_object.data')

    res = []
    for i in range(1000):
        tmp = []
        dat = dataGen(js)
        dat1 = dat[:]
        dat1[-3:] = list(map(int,dat[-3:]))
        engine.ingest(dat1)
        feat = engine.render(cid)
        # print(' '.join(map(str,feat)))
        tmp.append(chain1.predict_one(feat)[0])
        tmp.append(chain2.predict_one(feat)[0])
        tmp += getStat(js,dat)
        tmp += dat
        if i % 100==0:
            print('Finished the {}th iteration'.format(i))
        res.append(tmp)

    res = pd.DataFrame(res)
    print("Maximum difference between C++ and Python: {}".format(max((res[0]-res[2]).abs().max(),
                                                                     (res[1]-res[3]).abs().max())))
    print("Average prediction time: {} millisecond".format(res[4].mean()))
    print("Maximum prediction time: {} millisecond".format(res[4].max()))
    return res
