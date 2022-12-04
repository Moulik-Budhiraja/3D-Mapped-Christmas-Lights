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

    void updateColorFades(double dt) {
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

            // Fade from fromRGB to toRGB

            uint8_t progress = (int)(led.fadeProgress / led.fadeTime * 255);

            CRGB color = fadeTowardColor(fromRGB, toRGB, progress);

            // Back to HSV

            CHSV hsv = rgb2hsv_approximate(color);

            led.color.h = hsv.h;
            led.color.s = hsv.s;
            led.color.v = hsv.v;

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

    // Fade LEDs

    tree.updateColorFades(loopDelta / 1000.0);

    // Apply Changes
    tree.applyChanges(leds);

    FastLED.show();

    if (tree.on) {
        if (taskManager.taskCount == 0) {
            Serial.println("Running Animation Frame");
            TempAnimation();
        }
    }
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

void HandleNotFound() { server.send(404, "text/plain", "Not found"); }
