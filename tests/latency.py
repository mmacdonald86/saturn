import os

from mars.feature_extraction import FeatureComposer, FeatureEngine
from sklearn.ensemble import RandomForestClassifier
from sklearn.datasets import make_classification

from mars.models import BinaryRandomForestClassifier


def main():
    n_features = 1000
    x, y = make_classification(n_samples=1000, n_features=n_features,
                               n_informative=2, n_redundant=0,
                               random_state=0, shuffle=False)
    args = {'n_estimators': 1000, 'max_depth': 5, 'random_state': 0}

    clf = RandomForestClassifier(**args)
    clf.fit(x, y)

    test_x = [[0]*n_features]
    print(f"sklearn predict_proba: {clf.predict_proba(test_x)}")

    mars_model = BinaryRandomForestClassifier(**args)
    mars_model.fit(x, y)
    print(f"mars predict_one: {mars_model.predict_one([0]*n_features)}")
    print(f"mars predict_many: {mars_model.predict_many(test_x)}")

    mars_model.cc_dump(os.path.join(f"../model/model_object_f{n_features}_t{args['n_estimators']}.data"))


if __name__ == '__main__':
    main()
