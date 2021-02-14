## Run

To run the server use the following command:

```
bazel build -c opt //...
./bazel-bin/server/studio --logtostderr --train_data data/constraint_layout_github_v4_train.proto
```

## Test HTTP POST

```
curl -d '{"jsonrpc":"2.0", "method":"layout", "id": 1, "params": {"id": 2, "ref_device": {"width": 720, "height": 1280}, "devices": [{"width": 700, "height": 1200}], "layout": {"content_frame": {"location": [0,0,720,1280]}, "components": [{"location":[10,10,40,40], "type": "TextView", "attributes": {"{http://schemas.android.com/apk/res/android}id": "@+id/viewabs", "{http://schemas.android.com/apk/res/android}layout_width": "wrap_content", "{http://schemas.android.com/apk/res/android}layout_height": "100dp"} }, {"location":[50,10,40,40], "type": "Button", "attributes": {"{http://schemas.android.com/apk/res/android}id": "@+id/button", "{http://schemas.android.com/apk/res/android}layout_width": "wrap_content", "{http://schemas.android.com/apk/res/android}layout_height": "100dp"} }] }}}' -H "Content-Type: application/json" -X POST http://localhost:9017
```




```
//test request for oracle (check if all the specifications can be statisfied in one layout)
curl -d '{"jsonrpc":"2.0", "method":"screens", "id": 1, "params": {"screens": [{"resolution": [350, 630], "views": [[0, 0, 350, 582], [0, 0, 350, 582], [0, 0, 0, 0], [0, 24, 350, 582], [0, 24, 350, 582], [0, 53, 350, 553], [137, 53, 212, 87], [22, 87, 327, 392], [31, 392, 319, 426], [22, 446, 327, 491], [22, 446, 168, 491], [168, 446, 180, 491], [180, 446, 327, 491], [0, 501, 350, 553], [0, 0, 350, 24]]}, {"resolution": [360, 640], "views": [[0, 0, 360, 592], [0, 0, 360, 592], [0, 0, 0, 0], [0, 24, 360, 592], [0, 24, 360, 592], [0, 58, 360, 558], [142, 58, 217, 92], [27, 92, 332, 397], [36, 397, 324, 431], [27, 451, 332, 496], [27, 451, 173, 496], [173, 451, 185, 496], [185, 451, 332, 496], [0, 506, 360, 558], [0, 0, 360, 24]]}, {"resolution": [370, 650], "views": [[0, 0, 370, 602], [0, 0, 370, 602], [0, 0, 0, 0], [0, 24, 370, 602], [0, 24, 370, 602], [0, 63, 370, 563], [147, 63, 222, 97], [32, 97, 337, 402], [41, 402, 329, 436], [32, 456, 337, 501], [32, 456, 178, 501], [178, 456, 190, 501], [190, 456, 337, 501], [0, 511, 370, 563], [0, 0, 370, 24]]}]}}' -H "Content-Type: application/json" -X POST http://localhost:9020
```

```
//test request for oracle (trigger CEGIS iterations)
curl -d '{"jsonrpc":"2.0", "method":"oracle", "id": 1, "params": {"screens": [{"resolution": [350, 630], "views": [[0, 0, 350, 582], [0, 0, 350, 582], [0, 0, 0, 0], [0, 24, 350, 582], [0, 24, 350, 582], [0, 53, 350, 553], [137, 53, 212, 87], [22, 87, 327, 392], [31, 392, 319, 426], [22, 446, 327, 491], [22, 446, 168, 491], [168, 446, 180, 491], [180, 446, 327, 491], [0, 501, 350, 553], [0, 0, 350, 24]]}, {"resolution": [360, 640], "views": [[0, 0, 360, 592], [0, 0, 360, 592], [0, 0, 0, 0], [0, 24, 360, 592], [0, 24, 360, 592], [0, 58, 360, 558], [142, 58, 217, 92], [27, 92, 332, 397], [36, 397, 324, 431], [27, 451, 332, 496], [27, 451, 173, 496], [173, 451, 185, 496], [185, 451, 332, 496], [0, 506, 360, 558], [0, 0, 360, 24]]}, {"resolution": [370, 650], "views": [[0, 0, 370, 602], [0, 0, 370, 602], [0, 0, 0, 0], [0, 24, 370, 602], [0, 24, 370, 602], [0, 63, 370, 563], [147, 63, 222, 97], [32, 97, 337, 402], [41, 402, 329, 436], [32, 456, 337, 501], [32, 456, 178, 501], [178, 456, 190, 501], [190, 456, 337, 501], [0, 511, 370, 563], [0, 0, 370, 24]]}]}}' -H "Content-Type: application/json" -X POST http://localhost:9020
```
