# InferUI Baseline

```
$ time ./bazel-bin/inferui/eval/nogis/inferui_baseline --logtostderr --base_syn_fallback

Results:
	RobustSyn
Fully Correct: 594 / 1844 (32.2126%)
	Orientation Horizontal: 1408 / 1844 (76.3557%)
	Orientation Vertical: 786 / 1844 (42.6247%)
Success: 85 / 85, Inconsistent: 0, Failed: 0(timeout: 0, unsat: 0)
Fixed Views Stats: 0 / 922 (0%)
	RobustSyn+Opt
Fully Correct: 650 / 1844 (35.2495%)
	Orientation Horizontal: 1378 / 1844 (74.7289%)
	Orientation Vertical: 816 / 1844 (44.2516%)
Success: 85 / 85, Inconsistent: 0, Failed: 1(timeout: 1, unsat: 0)
Fixed Views Stats: 0 / 922 (0%)
	SingleSyn
Fully Correct: 260 / 1844 (14.0998%)
	Orientation Horizontal: 1044 / 1844 (56.6161%)
	Orientation Vertical: 474 / 1844 (25.705%)
Success: 85 / 85, Inconsistent: 0, Failed: 0(timeout: 0, unsat: 0)
Fixed Views Stats: 0 / 922 (0%)
	SingleSyn+Opt
Fully Correct: 464 / 1844 (25.1627%)
	Orientation Horizontal: 976 / 1844 (52.9284%)
	Orientation Vertical: 688 / 1844 (37.3102%)
Success: 85 / 85, Inconsistent: 0, Failed: 0(timeout: 0, unsat: 0)
Fixed Views Stats: 0 / 922 (0%)
	SingleSynOneQuery
Fully Correct: 286 / 1844 (15.5098%)
	Orientation Horizontal: 1008 / 1844 (54.6638%)
	Orientation Vertical: 486 / 1844 (26.3557%)
Success: 85 / 85, Inconsistent: 0, Failed: 0(timeout: 0, unsat: 0)
Fixed Views Stats: 0 / 922 (0%)
	SingleSynOneQuery+Opt
Fully Correct: 456 / 1844 (24.7289%)
	Orientation Horizontal: 992 / 1844 (53.7961%)
	Orientation Vertical: 680 / 1844 (36.8764%)
Success: 85 / 85, Inconsistent: 0, Failed: 0(timeout: 0, unsat: 0)
Fixed Views Stats: 0 / 922 (0%)
	UserFeedbackRobustSyn+Opt
Fully Correct: 1840 / 1844 (99.7831%)
	Orientation Horizontal: 1844 / 1844 (100%)
	Orientation Vertical: 1840 / 1844 (99.7831%)
Success: 85 / 85, Inconsistent: 1, Failed: 0(timeout: 0, unsat: 0)
Fixed Views Stats: 295 / 922 (31.9957%)
	UserFeedbackSingleSyn+Opt
Fully Correct: 1814 / 1844 (98.3731%)
	Orientation Horizontal: 1842 / 1844 (99.8915%)
	Orientation Vertical: 1816 / 1844 (98.4816%)
Success: 85 / 85, Inconsistent: 1, Failed: 0(timeout: 0, unsat: 0)
Fixed Views Stats: 522 / 922 (56.6161%)
```