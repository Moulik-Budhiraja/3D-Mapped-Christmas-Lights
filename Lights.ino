#include <FastLED.h>
#include <WebServer.h>
#include <WiFi.h>

#define NUM_LEDS 50
#define LED_PIN 25

#define MAX_TASKS 1900

const char* ssid = "REDACTED";
const char* password = "REDACTED";

WebServer server(80);

CRGB leds[NUM_LEDS];

int ledPos[50][4] = {
    {168, 53, 124, 0},   {158, 36, 94, 1},    {152, 37, 70, 2},
    {138, 33, 80, 3},    {113, 66, 52, 4},    {111, 29, 36, 5},
    {81, 45, 13, 6},     {74, 12, 18, 7},     {38, 13, 50, 8},
    {14, 34, 51, 9},     {3, 76, 29, 10},     {45, 90, 0, 11},
    {75, 81, 26, 12},    {105, 93, 49, 13},   {102, 131, 56, 14},
    {118, 101, 74, 15},  {101, 63, 101, 16},  {137, 112, 118, 17},
    {130, 112, 120, 18}, {128, 121, 133, 19}, {107, 143, 120, 20},
    {107, 169, 108, 21}, {131, 179, 106, 22}, {94, 171, 95, 23},
    {117, 160, 86, 24},  {105, 146, 61, 25},  {68, 160, 63, 26},
    {84, 132, 41, 27},   {78, 118, 79, 28},   {36, 173, 40, 29},
    {12, 116, 36, 30},   {12, 112, 85, 31},   {3, 151, 147, 32},
    {0, 179, 38, 33},    {29, 201, 51, 34},   {60, 172, 56, 35},
    {31, 200, 61, 36},   {60, 188, 88, 37},   {79, 215, 50, 38},
    {104, 209, 63, 39},  {118, 200, 92, 40},  {74, 226, 89, 41},
    {80, 217, 102, 42},  {80, 223, 130, 43},  {86, 251, 104, 44},
    {93, 236, 73, 45},   {59, 240, 64, 46},   {50, 232, 94, 47},
    {33, 264, 128, 48},  {72, 285, 97, 49}};

class TaskManager;

class THSV {
   public:
    uint8_t h;
    uint8_t s;
    uint8_t v;

    THSV() {
        h = 0;
        s = 0;
        v = 0;
    }
    THSV(uint8_t h, uint8_t s, uint8_t v) {
        this->h = h;
        this->s = s;
        this->v = v;
    }
};

class LED {
   public:
    int x;
    int y;
    int z;
    int pos;
    THSV color;
    THSV fadeFromColor = THSV(0, 0, 0);
    THSV fadeToColor = THSV(0, 0, 0);
    double fadeTime = -1;
    double fadeProgress = 0;

    LED() {
        x = 0;
        y = 0;
        z = 0;
        pos = 0;
        color.h = 0;
        color.s = 0;
        color.v = 0;
    }

    LED(int x, int y, int z, int position) {
        this->x = x;
        this->y = y;
        this->z = z;
        this->pos = position;
    }
};

class Task {
   public:
    uint8_t pos;
    THSV color;
    double delay = 0;
    THSV fadeToColor = THSV(0, 0, 0);
    double fadeTime = -1;
    bool complete = false;

    Task() {
        pos = 0;
        color.h = 0;
        color.s = 0;
        color.v = 0;
    }

    Task(uint8_t pos, THSV color, int delay) {
        this->pos = pos;
        this->color = color;
        this->delay = delay;
    }

    Task(uint8_t pos, THSV color, int delay, THSV fadeToColor, int fadeTime) {
        this->pos = pos;
        this->color = color;
        this->delay = delay;
        this->fadeToColor = fadeToColor;
        this->fadeTime = fadeTime;
    }

    Task copy() {
        Task t = Task(pos, color, delay, fadeToColor, fadeTime);
        return t;
    }
};

class Tree {
   public:
    LED ledPositions[NUM_LEDS];
    bool on = false;
    LED inArea[NUM_LEDS];

    Tree() {
        int i = 0;
        for (int* led : ledPos) {
            ledPositions[i] = LED(led[0], led[1], led[2], led[3]);
            i++;
        }
    }

    int getPos(int x, int y, int z) {
        int pos = -1;
        for (int i = 0; i < NUM_LEDS; i++) {
            if (ledPositions[i].x == x && ledPositions[i].y == y &&
                ledPositions[i].z == z) {
                pos = ledPositions[i].pos;
                break;
            }
        }
        return pos;
    }

    int getArea(int x1, int y1, int z1, int x2, int y2, int z2) {
        int count = 0;

        for (int i = 0; i < NUM_LEDS; i++) {
            if (ledPositions[i].x >= x1 && ledPositions[i].x <= x2 &&
                ledPositions[i].y >= y1 && ledPositions[i].y <= y2 &&
                ledPositions[i].z >= z1 && ledPositions[i].z <= z2) {
                inArea[count] = ledPositions[i];
                count++;
            }
        }

        return count;
    }

    void updateColorFades(double dt, CRGB leds[]) {
        for (int i = 0; i < NUM_LEDS; i++) {
            LED& led = ledPositions[i];

            if (led.fadeTime == -1) {
                continue;
            }

            led.fadeProgress += dt;

            if (led.fadeProgress >= led.fadeTime) {
                led.fadeProgress = led.fadeTime;
            }

            // HSV to RGB Conversion

            CHSV fromHSV(led.fadeFromColor.h, led.fadeFromColor.s,
                         led.fadeFromColor.v);
            CRGB fromRGB;
            hsv2rgb_rainbow(fromHSV, fromRGB);

            CHSV toHSV(led.fadeToColor.h, led.fadeToColor.s, led.fadeToColor.v);
            CRGB toRGB;
            hsv2rgb_rainbow(toHSV, toRGB);

            CHSV current(led.color.h, led.color.s, led.color.v);
            CRGB currentRGB;
            hsv2rgb_rainbow(current, currentRGB);

            // Fade from fromRGB to toRGB

            uint8_t progress = (int)(led.fadeProgress / led.fadeTime * 255);

            // if (led.pos == 17) {
            //     Serial.print("Color: ");
            //     Serial.print(currentRGB.r);
            //     Serial.print(", ");
            //     Serial.print(currentRGB.g);
            //     Serial.print(", ");
            //     Serial.print(currentRGB.b);
            //     Serial.print(" | Color After: ");
            // }

            CRGB color = fadeTowardColor(fromRGB, toRGB, progress);

            leds[led.pos] = color;

            // if (led.pos == 17) {
            //     // Serial.print("Color: ");
            //     Serial.print(color.r);
            //     Serial.print(", ");
            //     Serial.print(color.g);
            //     Serial.print(", ");
            //     Serial.println(color.b);
            // }

            // // Back to HSV

            CHSV hsv = rgb2hsv_approximate(color);

            led.color.h = hsv.h;
            led.color.s = hsv.s;
            led.color.v = hsv.v;

            // if (led.pos == 17) {
            //     Serial.print("  HSV Color: ");
            //     Serial.print(led.color.h);
            //     Serial.print(", ");
            //     Serial.print(led.color.s);
            //     Serial.print(", ");
            //     Serial.println(led.color.v);
            // }

            if (led.fadeProgress >= led.fadeTime) {
                led.fadeTime = -1;
            }
        }
    }

    void applyChanges(CRGB leds[]) {
        for (int i = 0; i < NUM_LEDS; i++) {
            LED led = ledPositions[i];

            leds[led.pos] = CHSV(led.color.h, led.color.s, led.color.v);
        }
    }

    void printDebugOutput() {
        for (int i = 0; i < NUM_LEDS; i++) {
            LED led = ledPositions[i];

            Serial.print(led.pos);
            Serial.print(": ");
            Serial.print(led.x);
            Serial.print(", ");
            Serial.print(led.y);
            Serial.print(", ");
            Serial.print(led.z);
            Serial.print(" - ");
            Serial.print(led.color.h);
            Serial.print(", ");
            Serial.print(led.color.s);
            Serial.print(", ");
            Serial.print(led.color.v);
            Serial.print(" - ");
            Serial.print(led.fadeToColor.h);
            Serial.print(", ");
            Serial.print(led.fadeToColor.s);
            Serial.print(", ");
            Serial.print(led.fadeToColor.v);
            Serial.print(" - ");
            Serial.print(led.fadeTime);
            Serial.print(" - ");
            Serial.println(led.fadeProgress);
        }
        Serial.println();
        Serial.println();
    }

   private:
    void nblendU8TowardU8(uint8_t& cur, const uint8_t target, uint8_t amount) {
        if (cur == target) return;

        if (cur < target) {
            uint8_t delta = target - cur;
            delta = delta * amount / 255;
            cur += delta;
        } else {
            uint8_t delta = cur - target;
            delta = delta * amount / 255;
            cur -= delta;
        }

        // cur = (int)(cur * (1 - (amount / 255)) + target * (amount / 255));
    }

    CRGB fadeTowardColor(CRGB& cur, const CRGB& target, uint8_t amount) {
        nblendU8TowardU8(cur.red, target.red, amount);
        nblendU8TowardU8(cur.green, target.green, amount);
        nblendU8TowardU8(cur.blue, target.blue, amount);
        return cur;
    }
};

class TaskManager {
   public:
    Task tasks[MAX_TASKS];
    int taskCount = 0;

    TaskManager() {}

    void addTask(Task task) {
        if (taskCount < MAX_TASKS) {
            tasks[taskCount] = task;
            // Serial.println("Added task");
            taskCount++;
        }
    }

    void addTask(int pos, THSV color, int delay) {
        tasks[taskCount] = Task(pos, color, delay);
        taskCount++;
    }

    void addTask(int pos, THSV color, int delay, THSV fadeToColor,
                 int fadeTime) {
        tasks[taskCount] = Task(pos, color, delay, fadeToColor, fadeTime);
        taskCount++;
    }

    void cleanTasks() {
        for (int i = 0; i < taskCount; i++) {
            if (tasks[i].complete) {
                for (int j = i; j < taskCount; j++) {
                    tasks[j] = tasks[j + 1];
                }
                taskCount--;
            }
        }
    }

    void clearTasks() {
        for (int i = 0; i < taskCount; i++) {
            tasks[i].complete = true;
        }
        cleanTasks();
    }

    void runTasks(Tree& tree) {
        for (int i = 0; i < taskCount; i++) {
            if (tasks[i].delay <= 0) {
                tree.ledPositions[tasks[i].pos].color = tasks[i].color;
                tree.ledPositions[tasks[i].pos].fadeFromColor = tasks[i].color;
                if (tasks[i].fadeTime >= 0) {
                    tree.ledPositions[tasks[i].pos].fadeToColor =
                        tasks[i].fadeToColor;
                    tree.ledPositions[tasks[i].pos].fadeTime =
                        tasks[i].fadeTime;
                    tree.ledPositions[tasks[i].pos].fadeProgress = 0;
                }
                tasks[i].complete = true;
            }
        }

        cleanTasks();
    }

    void printTasks() {
        for (int i = 0; i < taskCount; i++) {
            Serial.print("Task ");
            Serial.print(i);
            Serial.print(" - ");
            Serial.print(tasks[i].pos);
            Serial.print(" - ");
            Serial.print(tasks[i].color.h);
            Serial.print(", ");
            Serial.print(tasks[i].color.s);
            Serial.print(", ");
            Serial.print(tasks[i].color.v);
            Serial.print(" - ");
            Serial.print(tasks[i].delay);
            Serial.print(" - ");
        }
    }

    void updateTaskDelays(double dt) {
        for (int i = 0; i < taskCount; i++) {
            tasks[i].delay -= dt;
        }
    }
};

Tree tree;
TaskManager taskManager;

String currentldam =
    "0,134,255,255,1160,0,255,255,1000|0,0,255,255,2160,0,0,0,1000|1,134,255,"
    "255,1245,0,255,255,1000|1,0,255,255,2245,0,0,0,1000|2,134,255,255,1240,0,"
    "255,255,1000|2,0,255,255,2240,0,0,0,1000|3,134,255,255,1270,0,255,255,"
    "1000|3,0,255,255,2270,0,0,0,1000|4,134,255,255,1095,0,255,255,1000|4,0,"
    "255,255,2095,0,0,0,1000|5,134,255,255,1280,0,255,255,1000|5,0,255,255,"
    "2280,0,0,0,1000|6,134,255,255,1200,0,255,255,1000|6,0,255,255,2200,0,0,0,"
    "1000|7,134,255,255,1365,0,255,255,1000|7,0,255,255,2365,0,0,0,1000|8,134,"
    "255,255,1360,0,255,255,1000|8,0,255,255,2360,0,0,0,1000|9,134,255,255,"
    "1255,0,255,255,1000|9,0,255,255,2255,0,0,0,1000|10,134,255,255,1045,0,255,"
    "255,1000|10,0,255,255,2045,0,0,0,1000|11,134,255,255,975,0,255,255,1000|"
    "11,0,255,255,1975,0,0,0,1000|12,134,255,255,1020,0,255,255,1000|12,0,255,"
    "255,2020,0,0,0,1000|13,134,255,255,960,0,255,255,1000|13,0,255,255,1960,0,"
    "0,0,1000|14,134,255,255,770,0,255,255,1000|14,0,255,255,1770,0,0,0,1000|"
    "15,134,255,255,920,0,255,255,1000|15,0,255,255,1920,0,0,0,1000|16,134,255,"
    "255,1110,0,255,255,1000|16,0,255,255,2110,0,0,0,1000|17,134,255,255,865,0,"
    "255,255,1000|17,0,255,255,1865,0,0,0,1000|18,134,255,255,865,0,255,255,"
    "1000|18,0,255,255,1865,0,0,0,1000|19,134,255,255,820,0,255,255,1000|19,0,"
    "255,255,1820,0,0,0,1000|20,134,255,255,710,0,255,255,1000|20,0,255,255,"
    "1710,0,0,0,1000|21,134,255,255,580,0,255,255,1000|21,0,255,255,1580,0,0,0,"
    "1000|22,134,255,255,530,0,255,255,1000|22,0,255,255,1530,0,0,0,1000|23,"
    "134,255,255,570,0,255,255,1000|23,0,255,255,1570,0,0,0,1000|24,134,255,"
    "255,625,0,255,255,1000|24,0,255,255,1625,0,0,0,1000|25,134,255,255,695,0,"
    "255,255,1000|25,0,255,255,1695,0,0,0,1000|26,134,255,255,625,0,255,255,"
    "1000|26,0,255,255,1625,0,0,0,1000|27,134,255,255,765,0,255,255,1000|27,0,"
    "255,255,1765,0,0,0,1000|28,134,255,255,835,0,255,255,1000|28,0,255,255,"
    "1835,0,0,0,1000|29,134,255,255,560,0,255,255,1000|29,0,255,255,1560,0,0,0,"
    "1000|30,134,255,255,845,0,255,255,1000|30,0,255,255,1845,0,0,0,1000|31,"
    "134,255,255,865,0,255,255,1000|31,0,255,255,1865,0,0,0,1000|32,134,255,"
    "255,670,0,255,255,1000|32,0,255,255,1670,0,0,0,1000|33,134,255,255,530,0,"
    "255,255,1000|33,0,255,255,1530,0,0,0,1000|34,134,255,255,420,0,255,255,"
    "1000|34,0,255,255,1420,0,0,0,1000|35,134,255,255,565,0,255,255,1000|35,0,"
    "255,255,1565,0,0,0,1000|36,134,255,255,425,0,255,255,1000|36,0,255,255,"
    "1425,0,0,0,1000|37,134,255,255,485,0,255,255,1000|37,0,255,255,1485,0,0,0,"
    "1000|38,134,255,255,350,0,255,255,1000|38,0,255,255,1350,0,0,0,1000|39,"
    "134,255,255,380,0,255,255,1000|39,0,255,255,1380,0,0,0,1000|40,134,255,"
    "255,425,0,255,255,1000|40,0,255,255,1425,0,0,0,1000|41,134,255,255,295,0,"
    "255,255,1000|41,0,255,255,1295,0,0,0,1000|42,134,255,255,340,0,255,255,"
    "1000|42,0,255,255,1340,0,0,0,1000|43,134,255,255,310,0,255,255,1000|43,0,"
    "255,255,1310,0,0,0,1000|44,134,255,255,170,0,255,255,1000|44,0,255,255,"
    "1170,0,0,0,1000|45,134,255,255,245,0,255,255,1000|45,0,255,255,1245,0,0,0,"
    "1000|46,134,255,255,225,0,255,255,1000|46,0,255,255,1225,0,0,0,1000|47,"
    "134,255,255,265,0,255,255,1000|47,0,255,255,1265,0,0,0,1000|48,134,255,"
    "255,105,0,255,255,1000|48,0,255,255,1105,0,0,0,1000|49,134,255,255,0,0,"
    "255,255,1000|49,0,255,255,1000,0,0,0,1000|49,0,0,0,2925,0,0,0,-1|";

void setup() {
    Serial.begin(115200);

    // Connect to wifi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("Connected to the WiFi network");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Server routes
    server.on("/", HandleSingleLED);
    server.on("/area", HandleArea);
    server.on("/on", OnSequence);
    server.on("/off", OffSequence);
    server.on("/loop", ldamLoop);
    server.on("add", addldam);
    server.onNotFound(HandleNotFound);

    server.begin();

    // Initialize LED strip
    FastLED.addLeds<WS2811, LED_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setCorrection(TypicalPixelString);
    FastLED.setBrightness(200);
}

int fade = 0;
int fadeDir = 1;

unsigned long lastLoop = micros();

void loop() {
    int loopDelta = micros() - lastLoop;
    lastLoop = micros();

    server.handleClient();

    // Trigger Pending Tasks
    taskManager.updateTaskDelays(loopDelta / 1000.0);
    taskManager.runTasks(tree);

    // Apply Changes
    tree.applyChanges(leds);

    // Fade LEDs
    tree.updateColorFades(loopDelta / 1000.0, leds);

    FastLED.show();

    if (tree.on) {
        if (taskManager.taskCount == 0) {
            // Serial.println("Running Animation Frame");
            // TempAnimation();
            parseldam();
        }
    }
}

/**
 * @brief Parses the current ldam string into a list of tasks that are then
 * added to the task manager
 *
 * ldam string format:
 * pos,h,s,v,delay,fade_h,fade_s,fade_v,1000|pos,h,s,v,delay,fade_h,fade_s,fade_v,1000|...
 * Note: any value can be an "r" to indicate a random value between 0 and 255
 * and "k" to indicate a random value between 0 and 1000
 *
 */
void parseldam() {
    // Serial.println("Parsing ldam string");
    Task task;
    int valType = 0;  // 0 = pos, 1 = h, 2 = s, 3 = v, 4 = delay, 5 = fade_h, 6
                      // = fade_s, 7 = fade_v, 8 = fadeTime
    int val = 0;
    int totalTasks = 0;
    for (int i = 0; i < currentldam.length(); i++) {
        if (currentldam.charAt(i) == ',') {
            switch (valType) {
                case 0:
                    task.pos = val;
                    break;
                case 1:
                    task.color.h = val;
                    break;
                case 2:
                    task.color.s = val;
                    break;
                case 3:
                    task.color.v = val;
                    break;
                case 4:
                    task.delay = val;
                    break;
                case 5:
                    task.fadeToColor.h = val;
                    break;
                case 6:
                    task.fadeToColor.s = val;
                    break;
                case 7:
                    task.fadeToColor.v = val;
                    break;
                case 8:
                    task.fadeTime = val;
                    break;
            }

            valType++;
            val = 0;
        } else if (currentldam.charAt(i) == '|') {
            if (valType == 8) {
                task.fadeTime = val;
            }

            taskManager.addTask(task.copy());
            totalTasks++;

            valType = 0;
            val = 0;

        } else if (currentldam.charAt(i) == 'r') {
            val = random8();
        } else if (currentldam.charAt(i) == 'k') {
            val = random16(1000);
        } else {
            val = val * 10 + (currentldam.charAt(i) - '0');
        }
    }

    // Serial.println("Finished parsing ldam string");
    // Serial.print("Added ");
    // Serial.print(totalTasks);
    // Serial.println(" tasks to task manager");

    // Serial.print("Longest delay: ");
    // int longest = 0;
    // for (int i = 0; i < taskManager.taskCount; i++) {
    //     if (taskManager.tasks[i].delay > longest) {
    //         longest = taskManager.tasks[i].delay;
    //     }
    // }
    // Serial.println(longest);
}

void HandleSingleLED() {
    if (server.args() == 2) {
        uint8_t pos;
        THSV color;

        bool complete[] = {false, false};

        for (uint8_t i = 0; i < server.args(); i++) {
            String argName = server.argName(i);
            String value = server.arg(i);
            if (argName == "position") {
                pos = value.toInt();
                complete[0] = true;
            } else if (argName == "color") {
                char* token = strtok((char*)value.c_str(), ",");
                color.h = atoi(token);
                token = strtok(NULL, ",");
                color.s = atoi(token);
                token = strtok(NULL, ",");
                color.v = atoi(token);
                complete[1] = true;
            }
        }

        if (complete[0] && complete[1]) {
            taskManager.addTask(pos, color, 0);

            server.send(200, "text/html", "OK");
        } else {
            server.send(400, "text/html", "Bad Request");
        }
    } else {
        server.send(400, "text/html", "Bad Request");
    }
}

void HandleArea() {
    if (server.args() >= 7) {
        uint8_t pos;
        THSV color;

        bool complete[] = {false, false};

        int pos3d[] = {-1, -1, -1, -1, -1, -1};

        for (uint8_t i = 0; i < server.args(); i++) {
            String argName = server.argName(i);
            String value = server.arg(i);
            if (argName == "color") {
                char* token = strtok((char*)value.c_str(), ",");
                color.h = atoi(token);
                token = strtok(NULL, ",");
                color.s = atoi(token);
                token = strtok(NULL, ",");
                color.v = atoi(token);
                complete[0] = true;
            } else if (argName == "x1") {
                pos3d[0] = value.toInt();
            } else if (argName == "y1") {
                pos3d[1] = value.toInt();
            } else if (argName == "z1") {
                pos3d[2] = value.toInt();
            } else if (argName == "x2") {
                pos3d[3] = value.toInt();
            } else if (argName == "y2") {
                pos3d[4] = value.toInt();
            } else if (argName == "z2") {
                pos3d[5] = value.toInt();
            }

            for (int i = 0; i < 6; i++) {
                if (pos3d[i] == -1) {
                    break;
                }

                if (i == 5) {
                    complete[1] = true;
                }
            }
        }

        if (!complete[0] || !complete[1]) {
            server.send(400, "text/html", "Bad Request");
            return;
        }

        for (int i = 0; i < tree.getArea(pos3d[0], pos3d[1], pos3d[2], pos3d[3],
                                         pos3d[4], pos3d[5]);
             i++) {
            taskManager.addTask(tree.inArea[i].pos, color, 0);
        }

        server.send(200, "text/html", "OK");

    } else {
        server.send(400, "text/html", "Bad Request");
    }
}

void OnSequence() {
    int dy = 10;
    int passes = 2;

    if (tree.on) {
        return;
    }

    for (int y = 0; y < 400 * passes; y += dy) {
        for (int* pos : ledPos) {
            if (pos[1] >= y % 400 && pos[1] <= y % 400 + dy) {
                taskManager.addTask(pos[3], THSV(134, 255, 255), y,
                                    THSV(0, 0, 0), 300);
            }
        }
    }

    tree.on = true;

    server.send(200, "text/html", "OK");

    Serial.println("Got On Request");
}

void OffSequence() {
    int dy = 10;
    int passes = 2;

    if (!tree.on) {
        return;
    }

    tree.on = false;

    taskManager.clearTasks();

    for (int y = 0; y < 400 * passes; y += dy) {
        for (int* pos : ledPos) {
            if (pos[1] >= (400 * passes - y) % 400 &&
                pos[1] <= (400 * passes - y) % 400 + dy) {
                taskManager.addTask(pos[3], THSV(0, 255, 255), y, THSV(0, 0, 0),
                                    300);
            }
        }
    }

    server.send(200, "text/html", "OK");
}

void TempAnimation() {
    int dy = 10;
    int passes = 1;

    for (int y = 0; y < 400 * passes; y += dy) {
        for (int* pos : ledPos) {
            if (pos[1] >= (400 * passes - y) % 400 &&
                pos[1] <= (400 * passes - y) % 400 + dy) {
                taskManager.addTask(pos[3], THSV(134, 255, 255), y * 5,
                                    THSV(0, 0, 0), 1000);
            }
        }
    }
}

/**
 * @brief Updates the ldam string
 *
 */
void ldamLoop() {
    for (int i = 0; i < server.args(); i++) {
        if (server.argName(i) == "ldam") {
            currentldam = server.arg(i);

            Serial.println("Got ldam: " + currentldam);

            server.send(200, "text/html", "OK");

            if (!tree.on) {
                OnSequence();
            }

            return;
        }
    }

    server.send(400, "text/html", "Bad Request missing ldam");
}

void addldam() {
    for (int i = 0; i < server.args(); i++) {
        if (server.argName(i) == "ldam") {
            currentldam += server.arg(i);

            Serial.println("Got ldam: " + currentldam);

            server.send(200, "text/html", "OK");
            return;
        }
    }

    server.send(400, "text/html", "Bad Request missing ldam");
}

void HandleNotFound() { server.send(404, "text/plain", "Not found"); }
