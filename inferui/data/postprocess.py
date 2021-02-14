#{"filename":"activity_steps_got_goal.xml","packagename":"org.iggymedia.periodtracker","screens":[{"resolution":[1440,2560],"views":[[0,50,1440,274],[160,306,1280,1426],[64,1490,1376,1566],[64,1630,1376,1706],[0,2126,1440,2318]]},{"resolution":[1400,2520],"views":[[0,50,1400,274],[140,306,1260,1426],[64,1490,1336,1566],[64,1630,1336,1706],[0,2086,1400,2278]]},{"resolution":[1480,2600],"views":[[0,50,1480,274],[180,306,1300,1426],[64,1490,1416,1566],[64,1630,1416,1706],[0,2166,1480,2358]]}]}

#sort resulutions properly
#sort views by views size
#filter out views of the same size

import os
import json
import argparse

class View:
    def __init__(self, _id, coordinates):
        self.id = _id
        self.coordinates = coordinates 


def custom_key(dim_json):
    return dim_json["resolution"][0] * dim_json["resolution"][1]


def custom_key_size(view_object):
    width = view_object.coordinates[2] - view_object.coordinates[0]
    height = view_object.coordinates[3] - view_object.coordinates[1]
    return width * height, view_object.coordinates[0]

def sortByArea(views):
    resort = []
    views_objects = []
    for i, view in enumerate(views):
        views_objects.append(View(i, view))
    views_objects.sort(key=custom_key_size, reverse=True)
    #make sure that the content view stays the content view
    #resort.append(0)
    for views_object in views_objects:
        #if(views_object.id != 0):
            resort.append(views_object.id)
        #else:
            #print(views_object.coordinates[0], views_object.coordinates[1], views_object.coordinates[2], views_object.coordinates[3]) 

    #if(views_objects[0].id != 0):
        #print("Content View was not the largest view. Forced order.")
        #print(views_objects[0].coordinates[0], views_objects[0].coordinates[1], views_objects[0].coordinates[2], views_objects[0].coordinates[3]) 
        #print(views_objects[0].coordinates)
        #print(views[0])
        #exit(0)
    return resort


def resortByIds(array, ids):
    resorted = []
    for idx in ids:
        resorted.append(array[idx])
    return resorted

def modifyJson(app, cnt):
    if 'id' not in app:
        app["id"] = cnt
    resolutions = app["screens"]
    if len(resolutions) != 3:
        print("Expect 3 dimensions, got {} instead".format(len(resolutions)))
        return None
    resolutions.sort(key=custom_key)
    # resolutions are now sorted in the correct order, now sort the views according to the size of the middle resolution
    # assuming {x,y,xend,yend}
    resort = sortByArea(resolutions[1]["views"])
    for resolution in resolutions:
        resolution["views"] = resortByIds(resolution["views"], resort)

    return app


def main():
    parser = argparse.ArgumentParser("Normalize Dataset of views by sorting them in decreasing order of their size")
    parser.add_argument("file", default="./neural_oracle/D_S+/data.json", help="Input file path to process")
    parser.add_argument("out_file", default="./neural_oracle/D_S+/data_post.json", help="Output file path")
    args = parser.parse_args()

    assert os.path.exists(args.file)

    with open(args.file, 'r') as fin, open(args.out_file, 'w') as fout:
        for cnt, line in enumerate(fin):
            app = json.loads(line)
            app = modifyJson(app, cnt)
            if app is not None:
                fout.write(json.dumps(app) + '\n')

if __name__ == "__main__":
    main()
