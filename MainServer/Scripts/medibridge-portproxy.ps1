# =====================================================
# medibridge-portproxy.ps1 — Windows ↔ WSL 포트 노출 (관리자 권한)
# =====================================================
# WSL 의 IP 는 부팅마다 바뀌어 portproxy 매번 재등록 필요.
# 본 스크립트가 자동으로 WSL IP 를 받아 메인서버(8001) + 데이터 보관 PC(8004)
# 두 포트를 한 번에 등록한다.
#
# 사용:
#   1) PowerShell 을 "관리자 권한으로" 열기
#   2) Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass     # 한 번만
#   3) cd C:\Users\LMS\Desktop\Project\MediBridge\MainServer\Scripts
#   4) .\medibridge-portproxy.ps1
#
# 효과:
#   - 0.0.0.0:8001 (Windows LAN) → WSL IP:8001 (메인서버)
#   - 0.0.0.0:8004 (Windows LAN) → WSL IP:8004 (데이터 보관 PC)
#   - 방화벽 인바운드 8001/tcp · 8004/tcp 허용
# =====================================================

#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"

# 포트 정의 — 추가/변경 시 여기만 수정
$Ports = @(
    @{ Port = 8001; Name = "MediBridge MainServer"     },
    @{ Port = 8004; Name = "MediBridge DataStorage PC" }
)

Write-Host "[portproxy] WSL IP 조회..."
$wslIpRaw = (wsl hostname -I).Trim()
if ([string]::IsNullOrWhiteSpace($wslIpRaw)) {
    Write-Error "WSL 이 켜져 있지 않거나 IP 를 받지 못했습니다. WSL 안에서 medibridge-up.sh 먼저 실행하세요."
    exit 1
}
$wslIp = $wslIpRaw.Split(' ')[0]
Write-Host "[portproxy] WSL IP = $wslIp"
Write-Host ""

foreach ($entry in $Ports) {
    $Port     = $entry.Port
    $RuleName = $entry.Name

    Write-Host "----- 포트 $Port ($RuleName) -----"

    # 1) 기존 portproxy 항목 제거 (idempotent)
    netsh interface portproxy delete v4tov4 listenport=$Port listenaddress=0.0.0.0 2>$null | Out-Null

    # 2) 새 portproxy 등록
    netsh interface portproxy add v4tov4 `
        listenport=$Port listenaddress=0.0.0.0 `
        connectport=$Port connectaddress=$wslIp | Out-Null
    Write-Host "  portproxy: 0.0.0.0:$Port  →  ${wslIp}:$Port"

    # 3) 방화벽 규칙 (없으면 추가)
    $existing = netsh advfirewall firewall show rule name="$RuleName" 2>$null
    if ($LASTEXITCODE -ne 0) {
        netsh advfirewall firewall add rule `
            name="$RuleName" `
            dir=in action=allow protocol=TCP localport=$Port | Out-Null
        Write-Host "  방화벽: '$RuleName' 규칙 추가됨"
    } else {
        Write-Host "  방화벽: '$RuleName' 규칙 이미 존재"
    }
    Write-Host ""
}

# 4) 등록 결과 표시
Write-Host "============================================================"
Write-Host "  Windows LAN  →  WSL 포워딩 등록 완료"
Write-Host "============================================================"
netsh interface portproxy show v4tov4

# 5) Windows LAN IP 안내
Write-Host ""
$ipv4 = (Get-NetIPAddress -AddressFamily IPv4 -PrefixOrigin Manual,Dhcp |
         Where-Object { $_.IPAddress -notlike '127.*' -and $_.IPAddress -notlike '169.*' } |
         Select-Object -First 1).IPAddress
if ($ipv4) {
    Write-Host "Windows LAN IP : $ipv4"
    Write-Host "다른 PC 검증용 :"
    foreach ($entry in $Ports) {
        Write-Host "  curl http://${ipv4}:$($entry.Port)/health"
    }
}
