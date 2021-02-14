# Datasets

Set of script that produce datasets used to train/evaluation neural oracle.

## Iterative Synthesis 

```
./bazel-bin/inferui/datasets/iterative_dataset_gen --logtostderr --train_data data/github_top500_v2_full.proto --scaling_factor 2 --path data/neural_oracle/D_S+/data.json --max_candidates 8
```

## Layout

Given a set of existing layouts, render them on a device with different dimensions.

Input:
* GitHub Layouts (720x1280)
* PlayStore Layouts (720x1280)

Output:
* View positions for 2x larger devices
  * Device(1400, 2520)
  * Device(1440, 2560)
  * Device(1480, 2600)

```
./bazel-bin/inferui/datasets/scale_dataset_from_layout --logtostderr --test_data data/github_top500_v2_full.proto --train_data data/constraint_layout_playstore_v2_full.proto --out_file data/neural_oracle/D_P/github.json
./bazel-bin/inferui/datasets/scale_dataset_from_layout --logtostderr --train_data data/github_top500_v2_full.proto --test_data data/constraint_layout_playstore_v2_full.proto --out_file data/neural_oracle/D_P/playstore.json
```

the `--out_file out_file.json` specifies location of the file with results.


```
$ head -n 1 out_file.json

{
   "id" : "4",
   "reference_resolutions" : [
      [ 360, 640 ],
      [ 350, 630 ],
      [ 370, 650 ]
   ],
   "screens" : [
      {
         "resolution" : [ 1400, 2520 ],
         "views" : [
            [ 60, 316, 1340, 2104 ],
            [ 100, 356, 1300, 1884 ],
            [ 100, 1884, 1300, 2064 ],
            [ 100, 1924, 680, 2064 ],
            [ 720, 1924, 1300, 2064 ]
         ]
      },
      {
         "resolution" : [ 1440, 2560 ],
         "views" : [
            [ 80, 336, 1360, 2124 ],
            [ 120, 376, 1320, 1904 ],
            [ 120, 1904, 1320, 2084 ],
            [ 120, 1944, 700, 2084 ],
            [ 740, 1944, 1320, 2084 ]
         ]
      },
      {
         "resolution" : [ 1480, 2600 ],
         "views" : [
            [ 100, 356, 1380, 2144 ],
            [ 140, 396, 1340, 1924 ],
            [ 140, 1924, 1340, 2104 ],
            [ 140, 1964, 720, 2104 ],
            [ 760, 1964, 1340, 2104 ]
         ]
      }
   ]
}

```

## Synthesis

Here the layouts are synthesize based on input specification from multiple devices.
Then, once we have the layouts we render them on a device with different resolution. 

Input:
* Absolute view positions on 2 or 3 devices with resolution:
  * Device(350, 630)
  * Device(360, 640)
  * Device(370, 650)

Output:
* View positions for 4x larger devices
  * Device(1400, 2520)
  * Device(1440, 2560)
  * Device(1480, 2600)

Note that here its important to use `--scaling_factor 0.5` flag as the training data is in different resolution than the input specification.

```
./bazel-bin/inferui/datasets/scale_dataset_syn --logtostderr --train_data data/github_top500_v2_full.proto --scaling_factor 0.5 --path data/rendered_rico/2plus_resolutions.json --out_file data/neural_oracle/D_S+/data.json
```

where `--out_file out_file.json` specifies location of the file with results.
The output format additionally includes which device resolutions were used to synthesize the layout in `"reference_resolutions"` field.

```
$ head -n 1 out_file.json

{
   "filename" : "activity_create_profile.xml",
   "packagename" : "com.socialteinc.socialate",
   "screens" : [
      {
         "resolution" : [ 1440, 2560 ],
         "views" : [
            [ 0, 50, 1440, 274 ],
            [ 0, 274, 1440, 2318 ]
         ]
      },
      {
         "resolution" : [ 1400, 2520 ],
         "views" : [
            [ 0, 50, 1400, 274 ],
            [ 0, 274, 1400, 2278 ]
         ]
      },
      {
         "resolution" : [ 1480, 2600 ],
         "views" : [
            [ 0, 50, 1480, 274 ],
            [ 0, 274, 1480, 2358 ]
         ]
      }
   ]
}
```
