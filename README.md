# InferUI

## Project Structure
```
inferui-source
├── constraint_layout_solver/	# contains reference Java implementation of ConstrainLayout
├── inferui/					# contains the InferUI synthesizer [1]
```

## Installation

### Constain Layout Solver

For installation instructions and more details, please refer to `constraint_layout_solver/README.md`

### InferUI Core [1]

For installation instructions and more details, please refer to `inferui/README.md`

### Guiding Synthesizers [2]

```
git clone git@github.com:eth-sri/guiding-synthesizers.git
cd guiding-synthesizers
python3 -m venv venv
source venv/bin/activate
pip install wheel
pip install -e .
```

If you see the following error when installing matplotlib:

```
src/checkdep_freetype2.c:1:10: fatal error: ft2build.h: No such file or directory
```

try installing `sudo apt install libfreetype6-dev`.

If you see the following error:

```
ModuleNotFoundError: No module named 'Cython'
```

try `pip install --upgrade cython`

Then, Download and unzip pretrained models

```
./guidesyn/extractData.sh
```

Start the server using:

```
cd guidesyn
python core/server/server.py
```

## Usage

1. Compile and run the constraint layout server

```
cd constraint_layout_solver
./run.sh
```

2. Run baseline models
```
cd inferui
OMP_NUM_THREADS=8 time ./bazel-bin/inferui/eval/nogis/inferui_baseline --logtostderr --base_syn_fallback
```

Note, earlier versions of Z3 (i.e., 4.6.2) had a bug that caused crashes when using multiple threads.
To use a single threaded version, simply set OMP_NUM_THREADS=1.

This produces following output (including debug info):
```
	SingleSynOneQuery
Fully Correct: 266 / 1844 (14.4252%)
	Orientation Horizontal: 1034 / 1844 (56.0738%)
	Orientation Vertical: 496 / 1844 (26.898%)
Success: 85 / 85, Inconsistent: 0, Failed: 3(timeout: 3, unsat: 0)

	SingleSynOneQuery+Opt
Fully Correct: 462 / 1844 (25.0542%)
	Orientation Horizontal: 990 / 1844 (53.6876%)
	Orientation Vertical: 690 / 1844 (37.4187%)
Success: 85 / 85, Inconsistent: 0, Failed: 0(timeout: 0, unsat: 0)

	RobustSyn
Fully Correct: 684 / 1844 (37.0933%)
	Orientation Horizontal: 1472 / 1844 (79.8265%)
	Orientation Vertical: 848 / 1844 (45.987%)
Success: 85 / 85, Inconsistent: 0, Failed: 0(timeout: 0, unsat: 0)

	RobustSyn+Opt
Fully Correct: 680 / 1844 (36.8764%)
	Orientation Horizontal: 1378 / 1844 (74.7289%)
	Orientation Vertical: 852 / 1844 (46.2039%)
Success: 85 / 85, Inconsistent: 0, Failed: 3(timeout: 3, unsat: 0)

```

which corresponds to Table 1 from [2]:

| [2] | baseline | + prob model  | + prob model & robust |
| ------- |:-------------:| -----:| -----:|
| Table 1 | 15.5% | 24.7% | 35.2% |

The results based on the above script with Z3 4.8.11:

| this repo | SingleSynOneQuery | SingleSynOneQuery+Opt  | RobustSyn+Opt |
| ------- |:-------------:| -----:| -----:|
|        | 14.4% | 25.1% | 36.9% |

Note that the number are not exact since this version is slightly different and it uses newer Z3 version.

3. Run guided models [2]

Start guided server. Note that this requires downloading the pretrained models first.
```
cd guiding-synthesizers/
source venv/bin/activate
cd guidesyn
python core/server/server.py
```

Run synthesis:

```
cd inferui
OMP_NUM_THREADS=8 time ./bazel-bin/inferui/eval/nogis/nogis_iterative --logtostderr
```

This produces following output (including debug info):
```
CNN-ds+-dsplus-
Fully Correct: 618 / 1844 (33.5141%)
	Orientation Horizontal: 1046 / 1844 (56.7245%)
	Orientation Vertical: 842 / 1844 (45.6616%)
Success: 85 / 85, Inconsistent: 2, Failed: 2(timeout: 0, unsat: 2)

MLP-ds+-dsplus-
Fully Correct: 858 / 1844 (46.5293%)
	Orientation Horizontal: 1542 / 1844 (83.6226%)
	Orientation Vertical: 980 / 1844 (53.1453%)
Success: 85 / 85, Inconsistent: 0, Failed: 0(timeout: 0, unsat: 0)

doubleRNN-ds+-dsplus-
Fully Correct: 1168 / 1844 (63.3406%)
	Orientation Horizontal: 1438 / 1844 (77.9826%)
	Orientation Vertical: 1384 / 1844 (75.0542%)
Success: 85 / 85, Inconsistent: 0, Failed: 0(timeout: 0, unsat: 0)

ensembleRnnCnnBoth-ds+-dsplus-
Fully Correct: 1308 / 1844 (70.9328%)
	Orientation Horizontal: 1604 / 1844 (86.9848%)
	Orientation Vertical: 1438 / 1844 (77.9826%)
Success: 85 / 85, Inconsistent: 0, Failed: 0(timeout: 0, unsat: 0)

```

which corresponds to the D_S+ row from Table 2 from [2]:

| [2] | MLP | CNN  | RNN | RNN + CNN |
| ------- |:-------------:| -----:| -----:| -----:|
| Table 2: D_S+ | 20.7% | 33.2% | 63.4% | 71.0% |

The results based on the above script with Z3 4.8.11:

| this repo | MLP | CNN  | RNN | RNN + CNN |
| ------- |:-------------:| -----:| -----:| -----:|
|        | 46.5% | 33.5% | 64.3% | 70.9% |

Note that the number are not exact since this version is slightly different and it uses newer Z3 version.

## Bias

By default, all the experiments keep default value for bias = 0.5 (for centering constraints).
To allow synthesizing any values 0 <= bias <= 1, change the following [line](https://github.com/eth-sri/inferui/blob/master/inferui/inferui/synthesis/z3inference.h#L1332) from:

```
AddPositionConstraints(s, z3_views, true);
```

to 

```
AddPositionConstraints(s, z3_views, false);
```

which results in the following Z3 constraints:

```
if (fixed_bias) {
	s.add(view.GetBiasExpr() == s.ctx().real_val("0.5"));
} else {
	s.add(view.GetBiasExpr() >= 0);
	s.add(view.GetBiasExpr() <= 1);
}
```

Note that the ML model presented in [2] was trained only with fixed bias = 0.5 and using it with other values would require retraining.

## Logs

We include two logs of running the synthesizer for reference:

```
$ tail -n 20 inferui_baseline_z3_4-8-11.log 
	SingleSynOneQuery
Fully Correct: 266 / 1844 (14.4252%)
	Orientation Horizontal: 1034 / 1844 (56.0738%)
	Orientation Vertical: 496 / 1844 (26.898%)
Success: 85 / 85, Inconsistent: 0, Failed: 3(timeout: 3, unsat: 0)

	SingleSynOneQuery+Opt
Fully Correct: 462 / 1844 (25.0542%)
	Orientation Horizontal: 990 / 1844 (53.6876%)
	Orientation Vertical: 690 / 1844 (37.4187%)
Success: 85 / 85, Inconsistent: 0, Failed: 0(timeout: 0, unsat: 0)

	RobustSyn+Opt
Fully Correct: 680 / 1844 (36.8764%)
	Orientation Horizontal: 1378 / 1844 (74.7289%)
	Orientation Vertical: 852 / 1844 (46.2039%)
Success: 85 / 85, Inconsistent: 0, Failed: 3(timeout: 3, unsat: 0)
```

This logs also includes the user input:
```
View(0), [0, 0], [1440, 2560], width=1440, height=2560
View(1), [148, 672], [1292, 1792], width=1144, height=1120
View(2), [160, 712], [1280, 1752], width=1120, height=1040
View(3), [168, 720], [1272, 1504], width=1104, height=784
View(4), [168, 720], [1272, 1000], width=1104, height=280
View(5), [168, 780], [1272, 900], width=1104, height=120
View(6), [420, 1512], [1020, 1712], width=600, height=200
View(7), [168, 900], [804, 1000], width=636, height=100
```

And the synthesize layout:
```
{
	"layout" : 
	[
		{
			"android:id" : "parent",
			"android:layout_height" : "2560px",
			"android:layout_width" : "1440px"
		},
		{
			"android:id" : "@+id/view1",
			"android:layout_height" : "1120px",
			"android:layout_marginBottom" : "768px",
			"android:layout_marginRight" : "148px",
			"android:layout_width" : "1144px",
			"app:layout_constraintBottom_toBottomOf" : "parent",
			"app:layout_constraintRight_toRightOf" : "parent"
		},
		{
			"android:id" : "@+id/view2",
			"android:layout_height" : "1040px",
			"android:layout_width" : "1120px",
			"app:layout_constraintBottom_toBottomOf" : "@+id/view1",
			"app:layout_constraintLeft_toLeftOf" : "@+id/view1",
			"app:layout_constraintRight_toRightOf" : "@+id/view1",
			"app:layout_constraintTop_toTopOf" : "@+id/view1"
		},
		...
```

## References

[1] The InferUI synthesizer is based on the following work:

```
@inproceedings{bielik2018inferui, 
	title={Robust Relational Layouts Synthesis from Examples for Android}, 
	author={Bielik, Pavol and Fischer, Marc and Vechev, Martin}, 
	booktitle={ACM SIGPLAN Notices}, 
	year={2018}, 
	organization={ACM}
}
```

[2] The ML extension that learns to guide the synthesizer is based on:

```
@inproceedings{laich2020guiding,
title={Guiding Program Synthesis by Learning to Generate Examples},
author={Larissa Laich and Pavol Bielik and Martin Vechev},
booktitle={International Conference on Learning Representations},
year={2020},
url={https://openreview.net/forum?id=BJl07ySKvS}
}
```