/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * I dont know what this test does.
 */
public class Main {
    public static void float_017() {
        float f = 3.1415926535f;
        double d = 3.1415926535;
        //float fd = (float) d;
        //Float off = new Float(f);
        //Float ofd = new Float(d);
        System.out.println("base values: d=" + d + " f=" + f);
        System.out.println("base values: d=" + d + " f=" + f);
        System.out.println("base values: f=" + f + " d=" + d);
        //System.out.println("object values: off="
        //    + off.floatValue() + " ofd=" + ofd.floatValue());
    }
    public static void main(String args[]) {
        float_017();
    }
}
