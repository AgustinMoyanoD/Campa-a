$files = Get-ChildItem "assets/results/*.res"
foreach ($f in $files) {
    $c = Get-Content $f.FullName -Raw
    $c = $c -replace '(?m)^Result_Gold=(0\s*|(?!\d))$', ''
    $c = $c -replace '(?m)^Result_Infamy=(0|0\.0)\s*$', ''
    $c = $c -replace '(?m)^Result_Moral=(0\s*|(?!\d))$', ''
    $c = $c -replace '(?m)^Result_Food=(0|0\.0)\s*$', ''
    $c = $c -replace '(?m)^Result_Water=(0|0\.0)\s*$', ''
    $c = $c -replace '(?m)^Loss_Initiates=(0\s*|(?!\d))$', ''
    $c = $c -replace '(?m)^Loss_Soldiers=(0\s*|(?!\d))$', ''
    $c = $c -replace '(?m)^Loss_Sergeants=(0\s*|(?!\d))$', ''
    
    $c = $c -replace '(?m)^Result_Gold=(\d+)$', 'Effect=gold:+$1'
    $c = $c -replace '(?m)^Result_Gold=-(\d+)$', 'Effect=gold:-$1'
    
    $c = $c -replace '(?m)^Result_Infamy=([0-9.]+)$', 'Effect=infamy:+$1'
    $c = $c -replace '(?m)^Result_Moral=(\d+)$', 'Effect=moral:+$1'
    $c = $c -replace '(?m)^Result_Moral=-(\d+)$', 'Effect=moral:-$1'
    $c = $c -replace '(?m)^Result_Food=([0-9.]+)$', 'Effect=food:+$1'
    
    if ($f.Name -match "res_abn_car_003") { $c = $c -replace 'Loss_Initiates=2', 'Effect=initiates:+2' }
    if ($f.Name -match "res_abn_car_004") { $c = $c -replace 'Loss_Initiates=-1', 'Effect=initiates:-1' }
    if ($f.Name -match "res_amb_054") { 
        $c = $c -replace 'Loss_Initiates=10', 'Effect=initiates:-10' 
        $c = $c -replace 'Loss_Soldiers=3', 'Effect=soldiers:-3' 
    }
    if ($f.Name -match "res_amb_402") { 
        $c = $c -replace 'Loss_Initiates=20', 'Effect=initiates:-20'
        $c = $c -replace 'Loss_Soldiers=10', 'Effect=soldiers:-10'
        $c = $c -replace 'Loss_Sergeants=1', 'Effect=sergeants:-1'
    }
    if ($f.Name -match "res_thn_brn_005") { 
        $c = $c -replace 'Loss_Initiates=100', 'Effect=initiates:-ALL'
        $c = $c -replace 'Loss_Soldiers=100', 'Effect=soldiers:-ALL'
        $c = $c -replace 'Loss_Sergeants=100', 'Effect=sergeants:-ALL'
    }
    
    # Remove empty lines
    $c = $c -replace '(?m)^\s*\r?\n', ''
    $c | Set-Content -Encoding UTF8 $f.FullName
}

$events = Get-ChildItem "assets/events/*.evt"
foreach ($e in $events) {
    $c = Get-Content $e.FullName -Raw
    $c = $c -replace '(?m)^\[OPTION_2\]\r?\nText=Ignorarla\.$', "[OPTION_2]`nText=Ignorarla.`nResult_pool=NONE"
    $c = $c -replace '(?m)^\[OPTION_2\]\r?\nText=Darle bola al boludo de Eur.*$', "[OPTION_2]`nText=Darle bola al boludo de Eurílico.`nResult_pool=NONE"
    $c | Set-Content -Encoding UTF8 $e.FullName
}
