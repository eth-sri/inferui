package srl.inf.ethz.ch;

import android.support.constraint.solver.LinearSystem;
import android.support.constraint.solver.widgets.ConstraintAnchor;
import android.support.constraint.solver.widgets.ConstraintWidget;
import android.support.constraint.solver.widgets.ConstraintWidgetContainer;
import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.ParseException;

import java.io.File;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class LayoutUtil {

    static Object getField(Object obj, String name) throws Exception {
        java.lang.reflect.Field field = obj.getClass().getDeclaredField(name); //NoSuchFieldException
        field.setAccessible(true);
        return field.get(obj);
    }

    static void setFinalField(Object obj, String name, Object newValue) throws Exception {
        java.lang.reflect.Field field = obj.getClass().getDeclaredField(name); //NoSuchFieldException
        field.setAccessible(true);

        java.lang.reflect.Field modifiersField = java.lang.reflect.Field.class.getDeclaredField("modifiers");
        modifiersField.setAccessible(true);
        modifiersField.setInt(field, field.getModifiers() & ~java.lang.reflect.Modifier.FINAL);

        field.set(obj, newValue);
    }

    private static final int CHAIN_SPREAD = 0;
    private static final int CHAIN_SPREAD_INSIDE = 1;
    private static final int CHAIN_PACKED = 2;

    private static int getChainStyleId(String chainStyle) {
        switch (chainStyle) {
            case "packed":
            case "CHAIN_PACKED":
                return CHAIN_PACKED;

            case "spread":
            case "CHAIN_SPREAD":
                return CHAIN_SPREAD;

            case "spread_inside":
            case "CHAIN_SPREAD_INSIDE":
                return CHAIN_SPREAD_INSIDE;

            default:
                return CHAIN_PACKED;
        }
    }

    private static int ParseValue(String value) {
        if (value.endsWith("dp")) {
            float v = ParseFloat(value);
            if (Math.round(v) * 2 != 2 * v) {
                System.err.println("WARN: imprecise margin. Got float but only int is supported: " + value);
            }
            return Math.round(v * 2);
        } else if (value.endsWith("px")) {
            return ParseInt(value);
        } else {
            System.err.println("Unknown Value type: '" + value + "'");
            return 0;
        }
    }

    private static int ParseInt(String value) {
        String v = value.substring(0, value.length() - 2);
        return Integer.parseInt(v);
    }

    private static float ParseFloat(String value) {
        String v = value.substring(0, value.length() - 2);
        return Float.parseFloat(v);
    }

    private static float ParseValue(JSONObject result, String name, float defaultValue) {
        if (!result.containsKey(name)) {
            return defaultValue;
        }
        return ParseValue((String) result.get(name));
    }

    private static int ParseInt(JSONObject result, String name, int defaultValue) {
        if (!result.containsKey(name)) {
            return defaultValue;
        }
        return ParseInt((String) result.get(name));
    }

    private static int GetPadding(JSONObject result, ConstraintAnchor.Type type) {
        float value = 0;
        switch (type) {
            case LEFT:
                value = ParseValue(result, "android:paddingLeft", 0);
                break;
            case RIGHT:
                value = ParseValue(result,"android:paddingRight", 0);
                break;
            case TOP:
                value = ParseValue(result,"android:paddingTop", 0);
                break;
            case BOTTOM:
                value = ParseValue(result,"android:paddingBottom", 0);
                break;
            default:
                System.err.println("Unknown Padding type: '" + type + "'");
        }
        if (Math.round(value) != value) {
            System.err.println("WARN: imprecise padding. Got float but only int is supported: " + type + " = " + value);
        }
//        System.out.println(type.name() + ": " + value);
        return Math.round(value);
    }

    private static class Padding {
        int mPaddingLeft;
        int mPaddingTop;
        int mPaddingRight;
        int mPaddingBottom;

        public Padding(JSONObject jsonView) {
            mPaddingLeft = GetPadding(jsonView, ConstraintAnchor.Type.LEFT);
            mPaddingTop = GetPadding(jsonView, ConstraintAnchor.Type.TOP);
            mPaddingRight = GetPadding(jsonView, ConstraintAnchor.Type.RIGHT);
            mPaddingBottom = GetPadding(jsonView, ConstraintAnchor.Type.BOTTOM);
        }
    }

    private static int GetMargin(JSONObject result, ConstraintAnchor.Type type) {
        float value = 0;
        switch (type) {
            case LEFT:
                value = ParseValue(result, "android:layout_marginLeft", 0);
                break;
            case RIGHT:
                value = ParseValue(result,"android:layout_marginRight", 0);
                break;
            case TOP:
                value = ParseValue(result,"android:layout_marginTop", 0);
                break;
            case BOTTOM:
                value = ParseValue(result,"android:layout_marginBottom", 0);
                break;
            default:
                System.err.println("Unknown Margin type: '" + type + "'");
        }
        if (Math.round(value) != value) {
            System.err.println("WARN: imprecise margin. Got float but only int is supported: " + type + " = " + value);
        }
//        System.out.println(type.name() + ": " + value);
        return Math.round(value);
    }

    public static class ViewDimension {
        int value;
        ConstraintWidget.DimensionBehaviour behaviour;

        ViewDimension(String jsonValue) {
            if (jsonValue.equals("match_parent") || jsonValue.equals("fill_parent")) {
                this.value = 0;
                this.behaviour = ConstraintWidget.DimensionBehaviour.MATCH_PARENT;
            } else if (jsonValue.equals("0dp") || jsonValue.equals("0dip")) {
                this.value = 0;
                this.behaviour = ConstraintWidget.DimensionBehaviour.MATCH_CONSTRAINT;
            } else {
                this.value = ParseValue(jsonValue);
                this.behaviour = ConstraintWidget.DimensionBehaviour.FIXED;
            }
        }
    }

    private static Map<String, ConstraintWidget> GetWidgets(List<JSONObject> jsonViews) {
        HashMap<String, ConstraintWidget> widgets = new HashMap<>();
        for (JSONObject jsonView : jsonViews) {
            ViewDimension horizontal = new ViewDimension((String) jsonView.get("android:layout_width"));
            ViewDimension vertical = new ViewDimension((String) jsonView.get("android:layout_height"));
            ConstraintWidget widget = new ConstraintWidget(horizontal.value, vertical.value);
            widget.setHorizontalDimensionBehaviour(horizontal.behaviour);
            widget.setVerticalDimensionBehaviour(vertical.behaviour);

            String id = (String) jsonView.get("android:id");
            widget.setDebugName(id);
            widgets.put(id, widget);
        }
        return widgets;
    }

    private static void AddConstraints(ConstraintWidget widget, JSONObject jsonView, Map<String, ConstraintWidget> widgets) {
        if (jsonView.containsKey("app:layout_constraintVertical_chainStyle")) {
            String chainStyle = (String) jsonView.get("app:layout_constraintVertical_chainStyle");
            widget.setVerticalChainStyle(getChainStyleId(chainStyle));
        }

        if (jsonView.containsKey("app:layout_constraintHorizontal_chainStyle")) {
            String chainStyle = (String) jsonView.get("app:layout_constraintHorizontal_chainStyle");
            widget.setHorizontalChainStyle(getChainStyleId(chainStyle));
        }

        if (jsonView.containsKey("app:layout_constraintHorizontal_bias")) {
            float bias = Float.parseFloat((String)jsonView.get("app:layout_constraintHorizontal_bias"));
            widget.setHorizontalBiasPercent(bias);
        }

        if (jsonView.containsKey("app:layout_constraintVertical_bias")) {
            float bias = Float.parseFloat((String)jsonView.get("app:layout_constraintVertical_bias"));
            widget.setVerticalBiasPercent(bias);
        }

        if (jsonView.containsKey("app:layout_constraintRight_toRightOf")) {
            String targetId = (String) jsonView.get("app:layout_constraintRight_toRightOf");
            widget.connect(ConstraintAnchor.Type.RIGHT, widgets.get(targetId), ConstraintAnchor.Type.RIGHT, GetMargin(jsonView, ConstraintAnchor.Type.RIGHT));
        }
        if (jsonView.containsKey("app:layout_constraintRight_toLeftOf")) {
            String targetId = (String) jsonView.get("app:layout_constraintRight_toLeftOf");
            widget.connect(ConstraintAnchor.Type.RIGHT, widgets.get(targetId), ConstraintAnchor.Type.LEFT, GetMargin(jsonView, ConstraintAnchor.Type.RIGHT));
        }
        if (jsonView.containsKey("app:layout_constraintLeft_toRightOf")) {
            String targetId = (String) jsonView.get("app:layout_constraintLeft_toRightOf");
            widget.connect(ConstraintAnchor.Type.LEFT, widgets.get(targetId), ConstraintAnchor.Type.RIGHT, GetMargin(jsonView, ConstraintAnchor.Type.LEFT));
        }
        if (jsonView.containsKey("app:layout_constraintLeft_toLeftOf")) {
            String targetId = (String) jsonView.get("app:layout_constraintLeft_toLeftOf");
            widget.connect(ConstraintAnchor.Type.LEFT, widgets.get(targetId), ConstraintAnchor.Type.LEFT, GetMargin(jsonView, ConstraintAnchor.Type.LEFT));
        }

        if (jsonView.containsKey("app:layout_constraintTop_toTopOf")) {
            String targetId = (String) jsonView.get("app:layout_constraintTop_toTopOf");
            widget.connect(ConstraintAnchor.Type.TOP, widgets.get(targetId), ConstraintAnchor.Type.TOP, GetMargin(jsonView, ConstraintAnchor.Type.TOP));
        }
        if (jsonView.containsKey("app:layout_constraintTop_toBottomOf")) {
            String targetId = (String) jsonView.get("app:layout_constraintTop_toBottomOf");
            widget.connect(ConstraintAnchor.Type.TOP, widgets.get(targetId), ConstraintAnchor.Type.BOTTOM, GetMargin(jsonView, ConstraintAnchor.Type.TOP));
        }
        if (jsonView.containsKey("app:layout_constraintBottom_toTopOf")) {
            String targetId = (String) jsonView.get("app:layout_constraintBottom_toTopOf");
            widget.connect(ConstraintAnchor.Type.BOTTOM, widgets.get(targetId), ConstraintAnchor.Type.TOP, GetMargin(jsonView, ConstraintAnchor.Type.BOTTOM));
        }
        if (jsonView.containsKey("app:layout_constraintBottom_toBottomOf")) {
            String targetId = (String) jsonView.get("app:layout_constraintBottom_toBottomOf");
            widget.connect(ConstraintAnchor.Type.BOTTOM, widgets.get(targetId), ConstraintAnchor.Type.BOTTOM, GetMargin(jsonView, ConstraintAnchor.Type.BOTTOM));
        }
    }

    private static JSONArray WidgetToJSONArray(ConstraintWidget widget, int xOffset, int yOffset) {
        JSONArray res = new JSONArray();
        res.add((widget.getLeft() + xOffset));
        res.add((widget.getTop() + yOffset));
        res.add(widget.getWidth());
        res.add(widget.getHeight());
        return res;
    }

    private static JSONObject LayoutToJSON(List<JSONObject> jsonViews, ConstraintWidget container, Map<String, ConstraintWidget> widgets,
                                           int xOffset, int yOffset) {
        JSONArray widgetsData = new JSONArray();

        // Keep the same order of views as in request
        for (JSONObject result : jsonViews) {
            String id = (String) result.get("android:id");
            if (id.equals("parent")) {
                continue;
            }

            ConstraintWidget widget = widgets.get(id);

            JSONObject widgetObject = new JSONObject();
            widgetObject.put("id", id);
            widgetObject.put("location", WidgetToJSONArray(widget, xOffset, yOffset));
            widgetsData.add(widgetObject);
        }

        JSONObject contentData = new JSONObject();
        contentData.put("name", "android.support.v7.widget.ContentFrameLayout");
        contentData.put("location", WidgetToJSONArray(container, xOffset, yOffset));


        JSONObject responseData = new JSONObject();

        responseData.put("content_frame", contentData);
        responseData.put("components", widgetsData);

        return responseData;
    }

    public static JSONObject LayoutViews(JSONObject data) throws ParseException {

        // Add widgets
        List<JSONObject> jsonViews = (List<JSONObject>) data.get("layout");
        Map<String, ConstraintWidget> widgets = GetWidgets(jsonViews);

        Padding padding = new Padding(jsonViews.get(0));
        ConstraintWidget container = widgets.get("parent");
        ConstraintWidgetContainer root = new ConstraintWidgetContainer(
                0,
                0,
                container.getWidth(),
                container.getHeight());


        root.setDebugSolverName(root.getSystem(),"root");

        container.setWidth(container.getWidth() - padding.mPaddingLeft - padding.mPaddingRight);
        container.setHeight(container.getHeight() - padding.mPaddingTop - padding.mPaddingBottom);
        root.add(container);
        container.connect(ConstraintAnchor.Type.LEFT, root, ConstraintAnchor.Type.LEFT, padding.mPaddingLeft);
        container.connect(ConstraintAnchor.Type.TOP, root, ConstraintAnchor.Type.TOP, padding.mPaddingTop);


        // Add widget constraints
        for (JSONObject jsonView : jsonViews) {
            String id = (String) jsonView.get("android:id");
            if (id.equals("parent")) {
                continue;
            }

            ConstraintWidget widget = widgets.get(id);
            root.add(widget);
            AddConstraints(widget, jsonView, widgets);
        }

        // Solve
        root.layout();

        // Add ConstraintView parent
        int xOffset = ((Long) data.getOrDefault("x_offset", 0L)).intValue();
        int yOffset = ((Long) data.getOrDefault("y_offset", 0L)).intValue();
        return LayoutToJSON(jsonViews, root, widgets, xOffset, yOffset);
    }


}
