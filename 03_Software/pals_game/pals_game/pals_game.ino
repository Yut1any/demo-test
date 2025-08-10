#include <TFT_eSPI.h>

// 引用所有的素材文件
#include "homepage.h"
#include "pkq1.h"
#include "pkq2.h"
#include "pkq3.h"
#include "pkq1_mirrored.h"
#include "pkq2_mirrored.h"
#include "pkq3_mirrored.h"
#include "pikachu_bg.h"
#include "slot_normal.h"
#include "slot_large.h"
#include "icon_normal.h"
#include "icon_large.h"
#include "bar_bg.h"

TFT_eSPI tft = TFT_eSPI();

// --- 按键定义 ---
#define BUTTON_1_PIN 10 // 左/上
#define BUTTON_2_PIN 11 // 右/下
#define BUTTON_3_PIN 12 // 确认
#define BUTTON_4_PIN 13 // 模式选择

// --- 宠物动画设置 ---
#define PIKACHU_WIDTH   24  // 宠物宽度
#define PIKACHU_HEIGHT  24  // 宠物高度
#define PIKACHU_X       124 // 宠物在屏幕上的X坐标 
#define PIKACHU_Y       186 // 宠物在屏幕上的Y坐标 

// 宠物图片的透明色是白色，记录并跳过实现透明效果。
#define PIKACHU_TRANSPARENT_COLOR 0xFFFF 

// 把所有6帧动画的数组指针都放到一个大数组里
const unsigned short* pikachu_frames[] = {
  // 正常朝向的3帧
  pkq1_0,
  pkq2_0,
  pkq3_0,
  // 镜像翻转的3帧
  pkq1_mirrored_0, 
  pkq2_mirrored_0,
  pkq3_mirrored_0
};

// --- 动画与移动状态管理 ---
int current_frame_index = 0;   // 当前播放到第几帧 (0-5)
unsigned long last_frame_time = 0; // 上次换帧时间
int frame_interval = 250;      // 每帧间隔

// 移动相关的状态变量
bool moving_right = true;      // 当前是向右移动还是向左
int move_step = 0;             // 当前移动到了第几步 (用于实现平滑移动)
#define MOVE_RANGE_PIXELS 10   // 总共移动的范围（像素）
#define MOVE_STEPS 20          // 用20步来完成这段移动，步数越多越平滑
unsigned long last_move_time = 0; // 上次移动的时间
int move_interval = 50;        // 每移动一小步的间隔（毫秒）

// 基础位置，宠物将围绕这个点移动
#define PIKACHU_BASE_X 124
#define PIKACHU_BASE_Y 186

// --- UI布局与状态管理 ---
#define SLOT_NORMAL_SIZE 36
#define SLOT_LARGE_SIZE  42
#define ICON_NORMAL_SIZE 30
#define ICON_LARGE_SIZE  36
#define ICON_TRANSPARENT_COLOR 0xFFFF // 图标的透明色是白色

// 计算布局，上下各放5个格子
#define NUM_SLOTS_PER_ROW 5
const int topRowY = 10;
const int bottomRowY = 320 - SLOT_LARGE_SIZE - 5; // Y坐标要为大尺寸预留空间
int slotX[NUM_SLOTS_PER_ROW]; // X坐标数组


// UI状态变量
int cursor_pos = 0;      // 统一的光标位置, 0-4是顶部栏, 5-9是底部栏
int last_cursor_pos = 0; // 记住上一个光标的位置

// --- 绘图函数 ---
void drawSlot(int position) {
  // 根据统一的位置(0-9)计算出它在哪一行、哪一列
  int row = position / NUM_SLOTS_PER_ROW; // 0或1
  int col = position % NUM_SLOTS_PER_ROW; // 0-4

  bool is_selected = (position == cursor_pos);
  
  int x = slotX[col]; 
  int y = (row == 0) ? topRowY : bottomRowY;

  // 绘制逻辑
  if (is_selected) {
    tft.pushImage(x, y, SLOT_LARGE_SIZE, SLOT_LARGE_SIZE, slot_large_0);
    int icon_x = x + (SLOT_LARGE_SIZE - ICON_LARGE_SIZE) / 2;
    int icon_y = y + (SLOT_LARGE_SIZE - ICON_LARGE_SIZE) / 2;
    tft.pushImage(icon_x, icon_y, ICON_LARGE_SIZE, ICON_LARGE_SIZE, icon_large_0, ICON_TRANSPARENT_COLOR);
  } else {
    // 用于擦除的背景图数组叫 bar_bg_0
    tft.pushImage(x, y, SLOT_LARGE_SIZE, SLOT_LARGE_SIZE, bar_bg_0);
    int offset = (SLOT_LARGE_SIZE - SLOT_NORMAL_SIZE) / 2;
    tft.pushImage(x + offset, y + offset, SLOT_NORMAL_SIZE, SLOT_NORMAL_SIZE, slot_normal_0);
    int icon_offset_x = x + (SLOT_LARGE_SIZE - ICON_NORMAL_SIZE) / 2;
    int icon_offset_y = y + (SLOT_LARGE_SIZE - ICON_NORMAL_SIZE) / 2;
    tft.pushImage(icon_offset_x, icon_offset_y, ICON_NORMAL_SIZE, ICON_NORMAL_SIZE, icon_normal_0, ICON_TRANSPARENT_COLOR);
  }
}

// 绘制整个UI栏
void drawAllSlots() {
  for (int i = 0; i < NUM_SLOTS_PER_ROW * 2; i++) {
    drawSlot(i); // 按0-9的顺序绘制所有格子
  }
}





// --- 主程序 ---
void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(0);
  tft.setSwapBytes(true);

  // 初始化按键引脚
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BUTTON_4_PIN, INPUT_PULLUP);

  // 计算X坐标，使其居中
  int total_width = NUM_SLOTS_PER_ROW * SLOT_LARGE_SIZE;
  int gap = (240 - total_width) / (NUM_SLOTS_PER_ROW + 1);
  for (int i=0; i < NUM_SLOTS_PER_ROW; i++) {
    slotX[i] = gap + i * (SLOT_LARGE_SIZE + gap);
  }

  // --- 初始绘制 ---
  // 1. 画静态背景
  tft.pushImage(0, 0, 240, 320, homepage_0);
  
  // 2. 画所有UI栏
  drawAllSlots();
}


void loop() {
  // 保存上一帧的宠物位置，用于擦除
  static int last_pikachu_x = PIKACHU_BASE_X;
  static int last_pikachu_y = PIKACHU_BASE_Y;

  // --- 动画逻辑 ---
  if (millis() - last_frame_time > frame_interval) {
    if (moving_right) {
      current_frame_index++;
      if (current_frame_index < 3 || current_frame_index > 5) {
        current_frame_index = 3;
      }
    } else {
      current_frame_index++;
      if (current_frame_index > 2) {
        current_frame_index = 0;
      }
    }
    last_frame_time = millis();
  }

  // --- 移动逻辑 ---
  if (millis() - last_move_time > move_interval) {
    if (moving_right) {
      move_step++;
      if (move_step >= MOVE_STEPS) {
        moving_right = false;
      }
    } else {
      move_step--;
      if (move_step <= 0) {
        moving_right = true;
      }
    }

    // 计算当前宠物的X坐标 
    int current_pikachu_x = PIKACHU_BASE_X + (move_step * MOVE_RANGE_PIXELS) / MOVE_STEPS;
    
    // --- 擦除与重绘逻辑 ---
    // 1. 在上一帧的位置，画上准备好的背景小图，实现“擦除”
    // 注意：画的背景图尺寸比宠物大一点，确保完全覆盖
    tft.pushImage(last_pikachu_x - 1, last_pikachu_y - 1, 26, 26, pikachu_bg_0);

    // 2. 在新位置绘制当前动画帧
    tft.pushImage(current_pikachu_x, PIKACHU_BASE_Y, PIKACHU_WIDTH, PIKACHU_HEIGHT, 
                  pikachu_frames[current_frame_index], 
                  PIKACHU_TRANSPARENT_COLOR);
    
    // 3. 更新位置记忆，为下一帧的擦除做准备
    last_pikachu_x = current_pikachu_x;
    last_pikachu_y = PIKACHU_BASE_Y;

    last_move_time = millis();
  }
  // --- 按键输入与UI更新逻辑 ---
  bool ui_updated = false;
  last_cursor_pos = cursor_pos; // 记住旧位置

  if (digitalRead(BUTTON_2_PIN) == LOW) { // 右移
    cursor_pos++;
    // 从顶部最后一个(4)跳到底部第一个(5), 从底部最后一个(9)跳回顶部第一个(0)实现循环选择
    if (cursor_pos >= NUM_SLOTS_PER_ROW * 2) cursor_pos = 0;
    ui_updated = true;
    delay(150);
  }
  if (digitalRead(BUTTON_1_PIN) == LOW) { // 左移
    cursor_pos--;
    if (cursor_pos < 0) cursor_pos = (NUM_SLOTS_PER_ROW * 2) - 1;
    ui_updated = true;
    delay(150);
  }
  
  // 按键3和4现在是预留状态，后续可以添加逻辑
  if (digitalRead(BUTTON_3_PIN) == LOW) {
    // 确认键逻辑...
    Serial.printf("Confirmed item at position: %d\n", cursor_pos);
    delay(200);
  }

  // 如果UI状态有变化，则重绘受影响的格子
  if (ui_updated) {
    // 1. 重绘之前选中的格子，让它变回普通大小
    drawSlot(last_cursor_pos);
    
    // 2. 重绘当前选中的格子，让它放大
    drawSlot(cursor_pos);
  }
}