Benchmarking utilities
======================

The primary use-case of these programs is to benchmark the speed of a model. A secondary use-case is to test that the code runs end to end.

Try to benchmark both during development and after production training (and before production deployment). The production training pipeline should include a component that generate a testing dataset suitable for the corresponding benchmark program.


SVR model (deprecated)
----------------------

The benchmark program takes a directory that contains

```
- model_config.json
- model_object.data
- data_test/
|     |- column_list.txt
|     |- brand_ids.txt 
|     |- raw.txt
|     |- user_extlba/
|     |      |- brand_id_1.txt
|     |      |- brand_id_2.txt
|     |      |- ...           
```

`raw.txt` is a tab-separated csv file containing feature values *except* user-level SVR prediction. Usually this file should have at least 10 thousand rows. **This file is ignored (its presence is optional), because the model uses a single feature, which is 'user-level SVR prediction'.**

`column_list.txt` contains name/type pairs of features in `raw.txt`, in corresponding order. One feature per line. **This file is ignored (its presence is optional).**

`brand_ids.txt` contains brand IDs trained for the model. One brand per line. The file looks like this:

```
aaa
bbb
ccc
```

where 'aaa', 'bbb', 'ccc' are brand ID's.

`user_extlab` contains user-level SVR predictions corresponding to each row in `raw.txt`
(which is for a particular user) for the brand ID that is used as the file name. This directory contains files `aaa.txt`, `bbb.txt`, `ccc.txt`, corresponding to the content of the file `brand_ids.txt`.