@echo off
set "PATH=C:\Program Files\GNU Octave\Octave-10.3.0\mingw64\bin;%PATH%"
echo Compilando juego de estrategia tactica (Motor desacoplado + Juego Campana)...

set "ENGINE_SRCS=src\engine\core\hex_grid.c src\engine\core\pathfinding.c src\engine\core\config_parser.c src\engine\core\rng.c src\engine\core\day_clock.c src\engine\presentation\camera_controller.c src\engine\presentation\tween.c src\engine\presentation\sequence.c src\engine\presentation\screen_fx.c src\engine\presentation\audio_engine.c src\engine\presentation\asset_cache.c src\engine\ui\ui_skin.c src\engine\ui\glossary.c src\engine\ui\ui_widgets.c src\engine\systems\effect_system.c"

set "GAME_SRCS=src\game\models\balance_config.c src\game\models\hex_tile.c src\game\models\map_generator.c src\game\models\general.c src\game\models\army.c src\game\models\enemy.c src\game\models\combat.c src\game\models\modifier.c src\game\models\event.c src\game\models\ration.c src\game\models\city.c src\game\models\menu.c src\game\controllers\asset_manager.c src\game\controllers\game_master.c src\game\controllers\npc_controller.c src\game\controllers\event_flow.c src\game\controllers\movement_controller.c src\game\presentation\audio_manager.c src\game\views\map_view.c src\game\views\menu_view.c src\game\views\hud.c src\game\views\event_view.c src\game\views\draft_view.c src\game\views\combat_view.c src\game\views\city_view.c src\game\views\enemy_view.c src\game\views\npc_view.c src\game\views\ration_view.c src\game\views\ration_mock.c src\game\views\ui_cards.c src\game\views\setup_view.c src\game\views\endscreen_view.c src\game\views\river_confirm_view.c src\game\app\app.c"

gcc -Iinclude -Iinclude/engine -Iinclude/game -Iinclude/game/models -Iinclude/game/views -Iinclude/game/controllers src\main.c %ENGINE_SRCS% %GAME_SRCS% -Llib -lraylibdll -lopengl32 -lgdi32 -lwinmm -lm -o juego.exe
if %ERRORLEVEL% EQU 0 (
    echo [OK] Compilacion exitosa. Ejecutable generado: juego.exe
) else (
    echo [ERROR] Fallo la compilacion.
)
