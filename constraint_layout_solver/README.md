# Constraint Layout Solver

Constraint layout solver used in Android implemented as a webserver.

## Requirements

Install Java.

# Ubuntu 16.04 (LTS) uses OpenJDK 8 by default:
```
sudo apt install openjdk-8-jdk
```

# Ubuntu 18.04 (LTS) uses OpenJDK 11 by default:
```
sudo apt install openjdk-11-jdk
```

## Setup

Compile the project (tested ubuntu 16.04):

```bash
./build.sh
```

this should produce a JAR file located in `build/libs/layout-1.0-SNAPSHOT.jar`

If you see the following error:
```
FAILURE: Build failed with an exception.

* What went wrong:
Could not determine java version from '11.0.10'.
```

It means you are using newer version of Java.
In this case, please install gradle manually:
```
sudo apt-get install gradle
```

And then compile using:
```
gradle jar
```

## Usage

Run the server:

```bash
./run.sh
```

this starts a web server which listens at port `9100` 
The webserver accept JSON requests of the following format:

```
{
// The android studio as well as the actual device returns view location using the absolute coordinates on the screen.
// The layout solver returns relative view location with respect to the parent layout.
// In case the parent layout covers the whole screen, the locations returned by both approaches will be the same.
// However, in case the parent layout is smaller (e.g., due to notification bar or action bar rendered above the application) the values become different.  
// To correct for this change we can use 'x_offset' and 'y_offset' attributes which denote the offset of the parent layout with respect to the device screen.  
  "x_offset": 0, //(Optional): value denoting x offset of the layout
  "y_offset": 0, //(Optional): value denoting y offset of the layout
// The 'layout' defines list of views and their attributes.
  "layout": [
  // Each view is a dictionary containing attributes describing the view
  // By convention, the first view always corresponds to the ConstraintLayout while subsequent view are its children 
    {
      "android:id": "parent",
      // the relevant attributes for the ConstraintLayout are it's width and height
      // By changing these values we can simulate devices of different size
      "android:layout_width": "360dp",
      "android:layout_height": "360dp"
    },
    {
      "android:id": "view1",
      // Width and Height
      // Supported Values:
      
      // same size as the parent
      // "android:layout_width": "match_parent",
      
      // fixed size
      // "android:layout_width": "60dp",
      
      // dynamic size based on the constraints, only works if constrained from both size (e.g., both left and right edge of the view)
      // "android:layout_width": "0dp",
      
      "android:layout_width": "60dp",
      "android:layout_height": "60dp",
      
      // Constraints as written in Android (we use prefix 'app')
      // Supported Constraints:
      // android:layout_marginLeft
      // android:layout_marginRight
      // android:layout_marginTop
      // android:layout_marginBottom
      
      // app:layout_constraintHorizontal_bias
      // app:layout_constraintVertical_bias
      
      // app:layout_constraintRight_toRightOf
      // app:layout_constraintRight_toLeftOf
      // app:layout_constraintLeft_toRightOf
      // app:layout_constraintLeft_toLeftOf
      // app:layout_constraintTop_toTopOf
      // app:layout_constraintTop_toBottomOf
      // app:layout_constraintBottom_toTopOf
      // app:layout_constraintBottom_toBottomOf
      
      "app:layout_constraintRight_toRightOf": "parent", 
      "app:layout_constraintTop_toTopOf": "parent",
      "android:layout_marginLeft": "10dp"
    }
  ]
}
```

Below is a sample request:

```bash
curl -d '{"layout": [{"android:id": "parent", "android:layout_width": "360dp", "android:layout_height": "360dp"}, {"android:id":"view1", "android:layout_width": "60dp", "android:layout_height": "60dp", "app:layout_constraintRight_toRightOf": "parent", "app:layout_constraintTop_toTopOf": "parent", "android:layout_marginRight": "10dp"}]}' -H "Content-Type: application/json" -X POST localhost:9100/layout
```

and response:

```
{
  "components": [
    // Location of all the components.
    // The format is: [x_start, y_start, width, height]
    {"location": [580,0,120,120], "id": "view1"}
  ],
  // Location of the layout (first view in the request)
  "content_frame": {
    "name":"android.support.v7.widget.ContentFrameLayout","location":[0,0,720,720]
  }
}
```

Note that the server does not validate whether the input is correct. For example it's possible to give incomplete constraints or negative margins which are simply ignored.
