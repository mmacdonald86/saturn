import math
from mars.models import BinaryRandomForestClassifier
import numpy as np
import pandas as pd
import random
from mars.feature_extraction import Feature, OneHot, FeatureComposer, FeatureEngine
import json

columns = ['os','sp_user_gender','bundle']
osval = ['ios','andriod']
genderval = ['m','f','unknown']
bundleval = ["1","2","3","4","5"]

df = []

for i in range(30):
    random.seed(i)
    df.append([random.sample(osval,1)[0],random.sample(genderval,1)[0],random.sample(bundleval,1)[0]])

## create feature composer
with open('data/model_config.json') as config_file:
    config = json.load(config_file)

features = config['features']
engine = FeatureEngine(columns)
for spec in features:
    print(engine.add_feature(spec))

cid = engine.add_composer(features)

trn_dat = []
for f in df:
    for i, v in enumerate(f):
        engine.ingest_column(i,v)
    trn_dat.append(engine.render(cid))

trn_dat = np.array(trn_dat)
np.random.seed(100)
trn_lab = np.random.binomial(1,0.5,30)

mars_model = BinaryRandomForestClassifier(n_estimators = 20,
            max_depth = 4, random_state = 0)
mars_model.fit(trn_dat,trn_lab)
mars_model.cc_dump('data/wr_model_object.data')
