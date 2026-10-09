$content = Get-Content -Raw -Encoding UTF8 "src/main.c"
$start = $content.IndexOf('if (selectedTile != NULL && !playerArmy.isMoving)')
$end = $content.IndexOf('EndMode2D();')
$before = $content.Substring(0, $start)
$after = $content.Substring($end)
$new_content = $before + $new_block + "

        " + $after
$new_content | Set-Content -Encoding UTF8 "src/main.c"
