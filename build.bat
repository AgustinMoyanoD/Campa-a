@echo off
set "PATH=C:\Program Files\GNU Octave\Octave-10.3.0\mingw64\bin;%PATH%"
echo Compilando juego de estrategia tactica...
gcc -Iinclude -Iinclude/models -Iinclude/views -Iinclude/controllers src\main.c src\models\hex_tile.c src\models\map_generator.c src\models\general.c src\models\army.c src\models\enemy.c src\models\combat.c src\models\effect.c src\models\modifier.c src\models\event.c src\views\map_view.c src\presentation\tween.c src\presentation\sequence.c src\presentation\screen_fx.c src\presentation\audio_manager.c src\presentation\ui_widgets.c src\views\hud.c src\views\event_view.c src\views\combat_view.c src\views\enemy_view.c src\controllers\asset_manager.c src\controllers\camera_controller.c src\controllers\game_master.c -Llib -lraylibdll -lopengl32 -lgdi32 -lwinmm -lm -o juego.exe
if %ERRORLEVEL% EQU 0 (
    echo [OK] Compilacion exitosa. Ejecutable generado: juego.exe
) else (
    echo [ERROR] Fallo la compilacion.
)




