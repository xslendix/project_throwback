#include "WorldGen.hpp"
#include "common.hpp"

#include "FPCamera.h"
#include "raylib_ex.hpp"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#include "lines.hpp"

f64 screen_scale = 1;
f64 prevScreenScale = 1;

bool running = true;

Font font;

char msg[512];
i32 frame_counter_message = 0;
auto send_message(char const *message, i32 delay = 2) {
  frame_counter_message = 30 * delay;
  strcpy(msg, message);
}

u8 const SIGNAL_BAR_BULLET_SPACING = 5;
struct SignalBar {
  u8 spacing;
  i32 max = 0;
  Vector2 position; // Center.
  Texture2D tex;

  auto Unload() { UnloadTexture(tex); }

  auto GenerateTexture() {
    let img = GenImageColor(13, 13, TRANSPARENT);
    defer(UnloadImage(img));

    ImageDrawCircle(&img, img.width / 2, img.height / 2, img.width / 2, BLACK);
    ImageDrawFlood(&img, {roundf(img.width / 2.f), roundf(img.width / 2.f)},
                   WHITE);

    tex = LoadTextureFromImage(img);
  }

  auto Draw(i32 progress) {
    i32 i;
    let width = tex.width * max + (max - 1) * spacing;
    let element_width = tex.width + spacing;
    let x = position.x - width / 2.;
    let percentage = progress / (float)this->max;

    Color color;
    if (percentage < 0.33f)
      color = RED;
    else if (percentage < 0.66f)
      color = YELLOW;
    else
      color = GREEN;

    for (i = 0; i < max; i++) {
      if (i >= progress)
        color = BLACK;
      DrawTexture(tex, x + i * element_width, position.y - tex.height / 2.,
                  color);
    }

    DrawTextEx(font, "VIDEO",
               {position.x - MeasureTextEx(font, "VIDEO", 14, 2).x / 2,
                position.y - 20},
               14, 2, BLACK);
    DrawTextEx(font, "SIGNAL",
               {position.x - MeasureTextEx(font, "SIGNAL", 14, 2).x / 2,
                position.y + 10},
               14, 2, BLACK);
  }

  auto static Build(i32 max = 5, Vector2 position = {0, 0},
                    u8 spacing = SIGNAL_BAR_BULLET_SPACING) -> SignalBar {
    SignalBar bar;
    bar.max = max;
    bar.position = position;
    bar.spacing = spacing;
    bar.GenerateTexture();
    return bar;
  }
};

RenderTexture exploration_window;

struct SoundManager {
  vector<Sound> sfx_artifacts, sfx_woosh;
  vector<Music> bgm;

  f64 volume_music = .2f, volume_sfx = .9f;

  int now_playing = 0;

  auto PlayRandomArtifactSfx() {
    i32 idx = GetRandomValue(0, sfx_artifacts.size() - 1);
    TraceLog(LOG_INFO, "Playing artifact #%i", idx);
    PlaySoundMulti(sfx_artifacts.at(idx));
  }

  auto PlayRandomWooshSfx() {
    i32 idx = GetRandomValue(0, sfx_woosh.size() - 1);
    TraceLog(LOG_INFO, "Playing woosh #%i", idx);
    PlaySoundMulti(sfx_woosh.at(idx));
  }

  auto PlayMusic() {
    if (IsMusicStreamPlaying(bgm.at(now_playing))) {
      UpdateMusicStream(bgm.at(now_playing));
      return;
    } else {
      now_playing = GetRandomValue(0, bgm.size() - 1);
      PlayMusicStream(bgm.at(now_playing));
      TraceLog(LOG_INFO, "Now2playing: %i", now_playing);
    }
  }

  auto ApplyVolumeChanges() {
    for (let song : bgm)
      SetMusicVolume(song, volume_music);

    for (let sfx : sfx_woosh)
      SetSoundVolume(sfx, volume_sfx);

    for (let sfx : sfx_artifacts)
      SetSoundVolume(sfx, volume_sfx);
  }

  auto static Build() -> SoundManager {
    SoundManager manager;
    int i;

    for (i = 0; i < 6; i++) {
      if (i == 5)
        continue; // HACK: Just a workaround to remove an annoying one.

      manager.sfx_artifacts.push_back(
          LoadSound(TextFormat("assets/sfx/artifact_woosh%i.wav", i)));
    }

    for (i = 0; i < 1; i++) {
      manager.sfx_woosh.push_back(
          LoadSound(TextFormat("assets/sfx/woosh%i.wav", i)));
    }

    // HACK: This is scuffed, oh well...
    manager.bgm.push_back(LoadMusicStream("assets/music/em-selpan.mp3.mp3"));
    manager.bgm.push_back(LoadMusicStream("assets/music/em-sonor-03.mp3.mp3"));
    manager.bgm.push_back(LoadMusicStream("assets/music/em-xanthos.mp3.mp3"));
    manager.bgm.push_back(LoadMusicStream("assets/music/magmi_-1.wav.mp3"));
    manager.bgm.push_back(
        LoadMusicStream("assets/music/magmi_drone-03.mp3.mp3"));
    manager.bgm.push_back(LoadMusicStream("assets/music/m_murray_envy.mp3"));
    manager.bgm.push_back(
        LoadMusicStream("assets/music/thesoundfxguy_horror.wav.mp3"));

    manager.ApplyVolumeChanges();

    return manager;
  }
};

SoundManager sound_manager;

// FIXME: This leaks memory.
struct HeadAnimation {
  Texture2D head, bg;
  string str = "";

  auto Draw() {
    i8 y_off = sin(GetTime() / .5) * 5;
    DrawTexture(bg, 0, 0, WHITE);
    DrawTexture(head, bg.width / 2 - head.width / 2, y_off, WHITE);
  }

  auto Update() {}

  auto Speak(string &text) {}

  auto static Build() -> HeadAnimation {
    HeadAnimation head;

    head.bg = LoadTexture("assets/textures/bg.png");
    head.head = LoadTexture("assets/textures/head.png");

    return head;
  }
};

Texture2D cursor;

static RenderTexture2D target{0, {}, {}};

SignalBar signal_bar;
HeadAnimation head_animation;

FPCamera cam;

Shader fog_shader, noise_shader;
Light player_light;

f64 const PLAYER_COLLISION_RADIUS = .25f;
i32 const ARTIFACT_COUNT = 5;
f64 const NOISE_DISTANCE = 1.5f;
f64 const FOG_DENSITY = 1.f;

World world;
Image *artifact_photos[5];

auto AllArtifactsPhotographed() -> bool {
  for (let image : artifact_photos)
    if (image == nullptr)
      return false;

  return true;
}

u8 COLOR_ALPHA_MAX = 0xee;
#define COLOR_ 255, 255, 255

enum class Screen {
  MAIN_MENU,
  INTRO,
  GAME,
  OUTRO,
};
Screen current_screen = Screen::MAIN_MENU;

f64 dist = 999;
f64 _time = 0, _percentage_noise;
bool all_artifacts_photographed = false;
bool show_head_animation = false;
f64 timer = 0;
i32 ending_sequence_timer = 0;
auto UpdateGame() {
  i32 i, j;
  let old_pos = cam.CameraPosition;
  timer += GetFrameTime() * 1000;
  f64 const target_time = 1000 / 15.;

  if (timer >= target_time) {
    if (all_artifacts_photographed && ending_sequence_timer >= 30 &&
        ending_sequence_timer < 60) {
      cam.CameraPosition.y += .025f;
      cam.Angle.z += 10 * DEG2RAD;
      cam.ViewCamera.up = Vector3RotateByQuaternion(
          cam.ViewCamera.up, QuaternionFromAxisAngle({0, 0, 1}, .025f));
      cam.UseController = false;
      cam.UseKeyboard = false;
      if (_percentage_noise <= .95f)
        _percentage_noise += .05f;
    }
    if (ending_sequence_timer >= 100) {
      show_head_animation = true;
      if (_percentage_noise > .2f)
        _percentage_noise -= .2f;
      if (_percentage_noise < .25f)
        _percentage_noise = .2f;
    }
    if (ending_sequence_timer >= 130) {
      current_screen = Screen::OUTRO;
    }
    if (all_artifacts_photographed)
      ending_sequence_timer++;
    cam.Update();
    timer = 0;
  }

  SetShaderValue(fog_shader, fog_shader.locs[SHADER_LOC_VECTOR_VIEW],
                 &cam.GetCamera().position.x, SHADER_UNIFORM_VEC3);

  dist = 999;
  for (i = 0; i < ARTIFACT_COUNT; i++) {
    let dist2 = world.DistanceToObject(cam, i);
    dist = min(dist, dist2);
  }
  if (!all_artifacts_photographed) {
    _percentage_noise = 1.f - dist / NOISE_DISTANCE;
    _percentage_noise = max(_percentage_noise, .1f);
  }

  if (_percentage_noise > .15)
    if (!all_artifacts_photographed && round(GetRandomValue(0, 100)) == 0)
      sound_manager.PlayRandomArtifactSfx();

  if (IsKeyPressed(KEY_P)) {
    int object_index;
    for (object_index = 0; object_index < 5; object_index++) {
      if (artifact_photos[object_index] == nullptr) {
        auto image = (Image *)calloc(1, sizeof(Image));
        *image = LoadImageFromTexture(exploration_window.texture);
        artifact_photos[object_index] = image;
      }
    }
  }

  if (IsKeyPressed(KEY_Q)) {
    auto object_index = world.CheckObjectInFront(cam);
    if (object_index != -1) {
      if (artifact_photos[object_index] == nullptr) {
        auto image = (Image *)calloc(1, sizeof(Image));
        *image = LoadImageFromTexture(exploration_window.texture);
        artifact_photos[object_index] = image;
        if (GetRandomValue(0, 1) == 0)
          send_message("This should do it.");
        else
          send_message("Professional photographer...");
      } else {
        send_message("I already took a photo of this.");
      }
    } else {
      send_message("Not visible enough.");
    }

    if (!all_artifacts_photographed)
      all_artifacts_photographed = AllArtifactsPhotographed();
  }

  for (i = 0; i < WORLD_SIZE_GEN.x; i++) {
    for (j = 0; j < WORLD_SIZE_GEN.y; j++) {
      if (ColorToInt(GetImageColor(world.image, i, j)) != ColorToInt(WHITE))
        continue;

      Vector2 c_point = {
          (i * WORLD_SCALE) / WORLD_SIZE_GEN.x,
          (j * WORLD_SCALE) / WORLD_SIZE_GEN.y,
      };
      Vector2 cam_pos = {
          cam.CameraPosition.x,
          cam.CameraPosition.z,
      };

      if (CheckCollisionPointCircle(c_point, cam_pos, PLAYER_COLLISION_RADIUS))
        cam.CameraPosition = old_pos;
    }
  }
}

int flag = true;
auto DrawGame() {
  size_t i;

  BeginTextureMode(exploration_window);
  {
    ClearBackground(GRAY);
    cam.BeginMode3D();
    {
      DrawPlane({0, 0.001, 0}, {50, 50}, BLACK); // simple world plane

      DrawModel(world.model, {0, 0, 0}, WORLD_SCALE, WHITE);
      for (i = 0; i < world.artifacts->size(); i++) {
        let model = world.artifacts->at(i);
        Vector3 circle = {
            world.circles.at(i).position.x,
            0,
            world.circles.at(i).position.y,
        };
        DrawModel(model,
                  Vector3Divide(
                      Vector3Multiply(circle, {WORLD_SCALE, 1, WORLD_SCALE}),
                      WORLD_SIZE_GEN_V3),
                  .25, WHITE);
      }
    }
    cam.EndMode3D();

    if (show_head_animation)
      head_animation.Draw();
  }
  EndTextureMode();

  BeginTextureMode(target);
  {
    i32 const padding =
        (target.texture.width - exploration_window.texture.width) / 2;

    ClearBackground(DARKGRAY);

    DrawRectangle(2, 2, 256 - 4, 256 - 4, GRAY);

    DrawRectangle(padding - 2, padding - 2,
                  exploration_window.texture.width + 4,
                  exploration_window.texture.height + 4, DARKGRAY);

    DrawTexturePro(exploration_window.texture,
                   {0, 0, (f64)exploration_window.texture.width,
                    -(f64)exploration_window.texture.height},
                   {(f64)padding, (f64)padding,
                    (f64)exploration_window.texture.width,
                    (f64)exploration_window.texture.height},
                   {0, 0}, .0f, WHITE);

    signal_bar.Draw((1 - (_percentage_noise - .1f)) * (4 - 1) + 1);

    // DrawTexture(cursor, GetMouseX(), GetMouseY(), WHITE);
    // DrawTextureEx(cursor, {(f64)GetMouseX(), (f64)GetMouseY()}, 0, 1, WHITE);

    BeginShaderMode(noise_shader);
    DrawRectangle(padding, padding, exploration_window.texture.width,
                  exploration_window.texture.height, WHITE);
    EndShaderMode();

    if (frame_counter_message > 0) {
      DrawRectangle(padding, padding + exploration_window.texture.height - 20,
                    exploration_window.texture.width, 20, {0, 0, 0, 0xDD});
      DrawTextEx(font, msg,
                 {(f64)padding + 2,
                  (f64)padding + exploration_window.texture.height - 17},
                 14, 2, WHITE);
      frame_counter_message--;
    }
  }
  EndTextureMode();
}

u8 outro_line_index = 0;
auto UpdateOutro() {
  i32 ch;
  if ((ch = GetKeyPressed()) != 0) {
    TraceLog(LOG_INFO, "Key pressed: %i", ch);
    if (!(ch == '1' || ch == '2' || ch == '3') && ch < 258)
      outro_line_index++;
  }

  if (outro_line_index == 2) {
    _percentage_noise = .5;
    outro_line_index = 3;
  }

  if (outro_line_index > 3 && lines_outro[outro_line_index].empty()) {
    running = false;
#if defined(PLATFORM_WEB)
    emscripten_cancel_main_loop();
#endif
  }
}

auto DrawOutro() {
  BeginTextureMode(exploration_window);
  {
    ClearBackground(GRAY);

    head_animation.Draw();
  }
  EndTextureMode();

  BeginTextureMode(target);
  {
    i32 padding = (target.texture.width - exploration_window.texture.width) / 2;
    if (outro_line_index > 1) {
      padding = 0;
      ClearBackground(WHITE);
    } else {
      ClearBackground(DARKGRAY);
    }

    if (padding != 0) {
      DrawRectangle(2, 2, 256 - 4, 256 - 4, GRAY);

      DrawRectangle(padding - 2, padding - 2,
                    exploration_window.texture.width + 4,
                    exploration_window.texture.height + 4, DARKGRAY);

      DrawTexturePro(exploration_window.texture,
                     {0, 0, (f64)exploration_window.texture.width,
                      -(f64)exploration_window.texture.height},
                     {(f64)padding, (f64)padding,
                      (f64)exploration_window.texture.width,
                      (f64)exploration_window.texture.height},
                     {0, 0}, .0f, WHITE);

      signal_bar.Draw((1 - (_percentage_noise - .1f)) * (4 - 1) + 1);
    }

    BeginShaderMode(noise_shader);
    if (padding != 0) {
      DrawRectangle(padding, padding, exploration_window.texture.width,
                    exploration_window.texture.height, WHITE);
    } else {
      DrawRectangle(0, 0, target.texture.width, target.texture.height, WHITE);
    }
    EndShaderMode();

    if (padding != 0) {
      DrawRectangle(padding, padding + exploration_window.texture.height - 18,
                    exploration_window.texture.width, 20, {0, 0, 0, 0xDD});
      DrawTextEx(font, lines_outro[outro_line_index].c_str(),
                 {(f64)padding + 2,
                  (f64)padding + exploration_window.texture.height - 17},
                 14, 2, WHITE);
    } else {
      u8 const padding = 20;
      DrawTextBoxed(font, lines_outro[outro_line_index].c_str(),
                    {(f64)padding, (f64)padding, 256 - (f64)padding * 2,
                     256 - (f64)padding * 2},
                    16, 2, true, BLACK);

      u16 width_press_any_key = MeasureTextEx(font, "...", 16, 2).x;

      DrawTextEx(font, "...",
                 {(f64)256 / 2 - (float)width_press_any_key / 2,
                  256 - (f64)padding - 16},
                 16, 2, BLACK);
    }
  }
  EndTextureMode();
}

u8 current_line_intro = 0;
auto UpdateIntro() {
  i32 ch;
  if ((ch = GetKeyPressed()) != 0) {
    TraceLog(LOG_INFO, "Key pressed: %i", ch);
    if (!(ch == '1' || ch == '2' || ch == '3') && ch < 258)
      current_line_intro++;
  }

  if (lines_intro[current_line_intro].empty()) {
    // TODO: Switch to game screen.
    current_screen = Screen::GAME;
  }
}

char TEXT_PRESS_ANY_KEY[] = "...";
char TEXT_I_UNDERSTAND[] = "I understand.";
auto DrawIntro() {
  u8 const padding = 20;

  BeginTextureMode(target);
  {
    ClearBackground(BLACK);
    DrawTextBoxed(font, lines_intro[current_line_intro].c_str(),
                  {padding, padding, 256 - padding * 2, 256 - padding * 2}, 16,
                  2, true, WHITE);

    char *next_txt = TEXT_PRESS_ANY_KEY;
    if (current_line_intro == 6) {
      next_txt = TEXT_I_UNDERSTAND;
    }
    u16 width_press_any_key = MeasureTextEx(font, next_txt, 16, 2).x;

    DrawTextEx(font, next_txt,
               {256.f / 2 - (float)width_press_any_key / 2, 256 - padding - 16},
               16, 2, WHITE);
  }
  EndTextureMode();
}

i8 current_menu_option = 0;

auto UpdateMenu() {
  if (IsKeyPressed(KEY_UP) && current_menu_option > 0)
    current_menu_option--;
  if (IsKeyPressed(KEY_DOWN) && current_menu_option < 3)
    current_menu_option++;

  if (IsKeyPressed(KEY_LEFT) && current_menu_option == 1 &&
      sound_manager.volume_music > .1f) {
    sound_manager.volume_music -= .1f;
    sound_manager.ApplyVolumeChanges();
  }
  if (IsKeyPressed(KEY_RIGHT) && current_menu_option == 1 &&
      sound_manager.volume_music < 1) {
    sound_manager.volume_music += .1f;
    sound_manager.ApplyVolumeChanges();
  }

  if (IsKeyPressed(KEY_LEFT) && current_menu_option == 2 &&
      sound_manager.volume_sfx > .5f) {
    sound_manager.volume_sfx -= .1f;
    sound_manager.ApplyVolumeChanges();
  }
  if (IsKeyPressed(KEY_RIGHT) && current_menu_option == 2 &&
      sound_manager.volume_sfx < 1) {
    sound_manager.volume_sfx += .1f;
    sound_manager.ApplyVolumeChanges();
  }

  if (IsKeyPressed(KEY_ENTER)) {
    if (current_menu_option == 0)
      current_screen = Screen::INTRO;
    if (current_menu_option == 3)
      running = false;
  }
}

string const menu_items[]{"Start", "Music Volume: ", "Sfx Volume: ", "Quit"};

auto DrawMenu() {
  u8 const padding = 20;

  int i;

  BeginTextureMode(target);
  {
    ClearBackground(BLACK);

    let a = MeasureTextEx(font, "Project Throwback", 20, 2);
    DrawTextEx(font, "Project Throwback", {256.f / 2 - a.x / 2, (f64)padding},
               20, 2, WHITE);

    for (i = 0; i < 4; i++) {
      char const *current = menu_items[i].c_str();
      char const *txt;

      if (i == 1)
        current =
            TextFormat("%s%.0f%", current, sound_manager.volume_music * 100);
      if (i == 2)
        current =
            TextFormat("%s%.0f%", current, sound_manager.volume_sfx * 100);

      if (i == current_menu_option)
        txt = TextFormat("> %s <", current);
      else
        txt = current;

      u8 menu_item_width = MeasureTextEx(font, txt, 16, 2).x;

      DrawTextEx(font, txt,
                 {(f64)256 / 2 - (float)menu_item_width / 2,
                  padding + (f64)i * padding + (a.y + padding)},
                 16, 2, WHITE);
    }
  }
  EndTextureMode();
}

auto MainLoop() {
  _time++;
  // Scaling
  if (IsKeyPressed(KEY_ONE))
    screen_scale = 1;
  else if (IsKeyPressed(KEY_TWO))
    screen_scale = 2;
  else if (IsKeyPressed(KEY_THREE))
    screen_scale = 3;

#if defined(PLATFORM_WEB)
  if (IsKeyPressed(KEY_F))
    EM_ASM(Module.requestFullscreen(0, 0));
#endif

  if (screen_scale != prevScreenScale) {
    prevScreenScale = screen_scale;
    SetWindowSize(SCREEN_WIDTH * screen_scale, SCREEN_HEIGHT * screen_scale);
    SetMouseScale(1.0f / screen_scale, 1.0f / screen_scale);
  }

  sound_manager.PlayMusic();

  SetTraceLogLevel(LOG_WARNING);
  let time_noise = GetShaderLocation(noise_shader, "time");
  SetShaderValue(noise_shader, time_noise, &_time, SHADER_UNIFORM_FLOAT);
  let percentage_noise = GetShaderLocation(noise_shader, "percentage");
  SetShaderValue(noise_shader, percentage_noise, &_percentage_noise,
                 SHADER_UNIFORM_FLOAT);
  SetTraceLogLevel(LOG_INFO);

  ClearBackground(BLACK);

  switch (current_screen) {
  case Screen::MAIN_MENU:
    UpdateMenu();
    DrawMenu();
    break;
  case Screen::INTRO:
    UpdateIntro();
    DrawIntro();
    break;
  case Screen::GAME:
    UpdateGame();
    DrawGame();
    break;
  case Screen::OUTRO:
    UpdateOutro();
    DrawOutro();
    break;
  }

  // BeginTextureMode(target);
  // DrawTextureEx(cursor, {(f64)GetMouseX(), (f64)GetMouseY()}, 0, 1, WHITE);
  // EndTextureMode();

  BeginDrawing();
  {
    DrawTexturePro(
        target.texture,
        {0, 0, (f64)target.texture.width, -(f64)target.texture.height},
        {0, 0, (f64)target.texture.width * screen_scale,
         (f64)target.texture.height * screen_scale},
        {0, 0}, .0f, WHITE);
  }
  EndDrawing();
}

auto main(void) -> i32 {
#if !defined(DEBUG)
  SetTraceLogLevel(LOG_NONE);
#endif
  i32 i;

  for (i = 0; i < ARTIFACT_COUNT; i++)
    artifact_photos[i] = nullptr;

  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Project Throwback");
  defer(CloseWindow());

  SetAudioStreamBufferSizeDefault(6000);

  InitAudioDevice();
  defer(CloseAudioDevice());

  HideCursor();

  cursor = LoadTexture("assets/textures/cursor.png");

  vector<Model> artifacts;
  world = World::Build(WORLD_SIZE_GEN, &artifacts, ARTIFACT_COUNT);
  defer(world.Unload());

  head_animation = HeadAnimation::Build();

  target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  defer(UnloadRenderTexture(target));

  SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
  exploration_window = LoadRenderTexture(SCREEN_WIDTH - 20, SCREEN_HEIGHT - 80);
  SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

  cam.Setup(60, {0, 0, 0});
  cam.UseMouseX = false;
  cam.UseMouseY = false;
  let pos = world.circles.at(0).position;
  cam.CameraPosition.x = (pos.x * WORLD_SCALE) / WORLD_SIZE_GEN.x;
  cam.CameraPosition.z =
      ((pos.y - world.circles.at(0).radius * .75) * WORLD_SCALE) /
      WORLD_SIZE_GEN.y;

  fog_shader =
      LoadShader(TextFormat("assets/shaders/glsl%i/lighting.vs", GLSL_VERSION),
                 TextFormat("assets/shaders/glsl%i/fog.fs", GLSL_VERSION));
  defer(UnloadShader(fog_shader));

  let fog_density = GetShaderLocation(fog_shader, "fogDensity");
  SetShaderValue(fog_shader, fog_density, &FOG_DENSITY, SHADER_UNIFORM_FLOAT);

  noise_shader =
      LoadShader(0, TextFormat("assets/shaders/glsl%i/noise.fs", GLSL_VERSION));
  defer(UnloadShader(noise_shader));

  fog_shader.locs[SHADER_LOC_MATRIX_MODEL] =
      GetShaderLocation(fog_shader, "matModel");
  fog_shader.locs[SHADER_LOC_VECTOR_VIEW] =
      GetShaderLocation(fog_shader, "viewPos");

  world.model.materials[0].shader = fog_shader;

  signal_bar = SignalBar::Build(5, {256 / 2., 256 * .85});

  for (i = 0; i < ARTIFACT_COUNT; i++) {
    let new_model =
        LoadModel(TextFormat("assets/models/artifact%i.blend.obj", i));
    new_model.materials[0].shader = fog_shader;
    artifacts.push_back(new_model);
  }

  sound_manager = SoundManager::Build();

  font = LoadFontEx("assets/ubuntu.ttf", 30, nullptr, 0);

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(MainLoop, 30, 1);
#else
  SetTargetFPS(30);
  while (!WindowShouldClose() && running) {
    MainLoop();
  }
#endif

  StopSoundMulti();

  for (auto artifact : artifacts)
    UnloadModel(artifact);
  for (auto image : artifact_photos) {
    if (image == nullptr)
      continue;
    UnloadImage(*image);
    free(image);
  }
  return 0;
}
