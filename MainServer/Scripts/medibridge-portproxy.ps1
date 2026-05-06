# =====================================================
# medibridge-portproxy.ps1 — Windows ↔ WSL 포트 노출 (관리자 권한)
# =====================================================
# WSL 의 IP 는 부팅마다 바뀌어 portproxy 매번 재등록 필요.
# 본 스크립트가 자동으로 WSL IP 를 받아 등록한다.
#
# 사용:
#   1) PowerShell 을 "관리자 권한으로" 열기
#   2) Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass     # 한 번만
#   3) cd C:\Users\LMS\Desktop\Project\MediBridge\MainServer\Scripts
#   4) .\medibridge-portproxy.ps1
#
# 효과:
#   - 0.0.0.0:8001 (Windows LAN) → WSL IP:8001 로 포워딩
#   - 방화벽 인바운드 8001/tcp 허용
# =====================================================

#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"
$Port = 8001

Write-Host "[portproxy] WSL IP 조회..."
$wslIpRaw = (wsl hostname -I).Trim()
if ([string]::IsNullOrWhiteSpace($wslIpRaw)) {
    Write-Error "WSL 이 켜져 있지 않거나 IP 를 받지 못했습니다. WSL 안에서 medibridge-up.sh 먼저 실행하세요."
    exit 1
}
$wslIp = $wslIpRaw.Split(' ')[0]
Write-Host "[portproxy] WSL IP = $wslIp"

# 1) 기존 portproxy 항목 제거 (idempotent)
netsh interface portproxy delete v4tov4 listenport=$Port listenaddress=0.0.0.0 2>$null | Out-Null

# 2) 새 portproxy 등록
netsh interface portproxy add v4tov4 `
    listenport=$Port listenaddress=0.0.0.0 `
    connectport=$Port connectaddress=$wslIp | Out-Null

# 3) 방화벽 규칙 (없으면 추가)
$ruleName = "MediBridge MainServer"
$existing = netsh advfirewall firewall show rule name="$ruleName" 2>$null
if ($LASTEXITCODE -ne 0) {
    netsh advfirewall firewall add rule `
        name="$ruleName" `
        dir=in action=allow protocol=TCP localport=$Port | Out-Null
    Write-Host "[portproxy] 방화벽 규칙 '$ruleName' 추가됨"
} else {
    Write-Host "[portproxy] 방화벽 규칙 '$ruleName' 이미 존재"
}

# 4) 등록 결과 표시
Write-Host ""
Write-Host "============================================================"
Write-Host "  Windows 0.0.0.0:$Port  →  WSL ${wslIp}:$Port  포워딩 등록 완료"
Write-Host "============================================================"
netsh interface portproxy show v4tov4

# 5) Windows LAN IP 안내
Write-Host ""
$ipv4 = (Get-NetIPAddress -AddressFamily IPv4 -PrefixOrigin Manual,Dhcp |
         Where-Object { $_.IPAddress -notlike '127.*' -and $_.IPAddress -notlike '169.*' } |
         Select-Object -First 1).IPAddress
if ($ipv4) {
    Write-Host "Windows LAN IP : $ipv4"
    Write-Host "팀원 안내용     : http://${ipv4}:$Port/health"
}
