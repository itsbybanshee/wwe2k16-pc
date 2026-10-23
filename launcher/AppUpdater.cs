using System.Diagnostics;
using System.IO.Compression;
using System.Net.Http.Headers;
using System.Text.Json;

namespace Wwe2k16Launcher;

internal sealed record ReleaseInfo(string Version, string Name, string Notes, Uri Asset, string? Sha256);

internal static class AppUpdater
{
    const string Api = "https://api.github.com/repos/itsbybanshee/wwe2k16-pc/releases/latest";
    static readonly HttpClient Http = CreateClient();
    public static string CurrentVersion => typeof(AppUpdater).Assembly.GetName().Version?.ToString(3) ?? "1.0.0";

    static HttpClient CreateClient()
    {
        var client = new HttpClient { Timeout = TimeSpan.FromSeconds(20) };
        client.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("WWE2K16Launcher", "1.0"));
        client.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
        return client;
    }

    public static async Task<ReleaseInfo?> CheckAsync()
    {
        using var response = await Http.GetAsync(Api);
        response.EnsureSuccessStatusCode();
        using var json = JsonDocument.Parse(await response.Content.ReadAsStringAsync());
        var root = json.RootElement;
        if (root.TryGetProperty("draft", out var draft) && draft.GetBoolean()) return null;
        var tag = root.GetProperty("tag_name").GetString() ?? "";
        var version = tag.TrimStart('v', 'V');
        if (!Version.TryParse(version, out var latest) || !Version.TryParse(CurrentVersion, out var current))
            throw new InvalidDataException("Release version must use a numeric tag such as v3.1.0.");
        if (latest <= current) return null;
        var assets = root.GetProperty("assets").EnumerateArray();
        foreach (var asset in assets)
        {
            var name = asset.GetProperty("name").GetString() ?? "";
            if (!name.EndsWith(".zip", StringComparison.OrdinalIgnoreCase)) continue;
            return new ReleaseInfo(version,
                root.TryGetProperty("name", out var releaseName) ? releaseName.GetString() ?? tag : tag,
                root.TryGetProperty("body", out var body) ? body.GetString() ?? "" : "",
                new Uri(asset.GetProperty("browser_download_url").GetString()!),
                asset.TryGetProperty("digest", out var digest) ? digest.GetString()?.Replace("sha256:", "", StringComparison.OrdinalIgnoreCase) : null);
        }
        throw new InvalidDataException("The latest GitHub release has no ZIP asset.");
    }

    public static async Task DownloadAndInstallAsync(Form owner, ReleaseInfo release)
    {
        using var response = await Http.GetAsync(release.Asset, HttpCompletionOption.ResponseHeadersRead);
        response.EnsureSuccessStatusCode();
        string staging = Path.Combine(Path.GetTempPath(), "WWE2K16-update-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(staging);
        string zip = Path.Combine(staging, "release.zip");
        await using (var output = File.Create(zip)) await response.Content.CopyToAsync(output);
        if (!string.IsNullOrWhiteSpace(release.Sha256))
        {
            using var stream = File.OpenRead(zip);
            var actual = Convert.ToHexString(await System.Security.Cryptography.SHA256.HashDataAsync(stream));
            if (!actual.Equals(release.Sha256, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("The downloaded release checksum does not match GitHub's digest.");
        }

        using (var archive = ZipFile.OpenRead(zip))
        {
            foreach (var entry in archive.Entries)
            {
                var rel = entry.FullName.Replace('\\', '/');
                if (rel.StartsWith('/') || rel.Split('/').Any(part => part == ".."))
                    throw new InvalidDataException("The release ZIP contains an unsafe path.");
                if (rel.Equals("WWE 2K16 PC.exe", StringComparison.OrdinalIgnoreCase) || rel.StartsWith("bin/", StringComparison.OrdinalIgnoreCase))
                {
                    var dest = Path.GetFullPath(Path.Combine(staging, "payload", rel.Replace('/', Path.DirectorySeparatorChar)));
                    Directory.CreateDirectory(Path.GetDirectoryName(dest)!);
                    if (entry.Length > 0) entry.ExtractToFile(dest, true);
                }
            }
        }
        string payload = Path.Combine(staging, "payload");
        if (!File.Exists(Path.Combine(payload, "WWE 2K16 PC.exe")) || !File.Exists(Path.Combine(payload, "bin", "ui.dat")))
            throw new InvalidDataException("This release ZIP is missing launcher files.");

        string script = Path.Combine(staging, "apply-update.ps1");
        string installRoot = AppContext.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar);
        string scriptText = "param($PidToWait,$Source,$Target)\n" +
            "try { Wait-Process -Id ([int]$PidToWait) -Timeout 120 -ErrorAction Stop } catch {}\n" +
            "$ErrorActionPreference='Stop'\n" +
            "Copy-Item -LiteralPath (Join-Path $Source 'WWE 2K16 PC.exe') -Destination (Join-Path $Target 'WWE 2K16 PC.exe') -Force\n" +
            "$srcBin=Join-Path $Source 'bin'; $dstBin=Join-Path $Target 'bin'; New-Item -ItemType Directory -Force -Path $dstBin | Out-Null\n" +
            "Get-ChildItem -LiteralPath $srcBin -File -Recurse | ForEach-Object { $rel=$_.FullName.Substring($srcBin.Length).TrimStart('\\'); $dst=Join-Path $dstBin $rel; New-Item -ItemType Directory -Force -Path (Split-Path $dst) | Out-Null; Copy-Item -LiteralPath $_.FullName -Destination $dst -Force }\n" +
            "Start-Process -FilePath (Join-Path $Target 'WWE 2K16 PC.exe') -WorkingDirectory $Target\n";
        await File.WriteAllTextAsync(script, scriptText);
        var start = new ProcessStartInfo("powershell.exe") { UseShellExecute = false, CreateNoWindow = true };
        start.ArgumentList.Add("-NoProfile"); start.ArgumentList.Add("-ExecutionPolicy"); start.ArgumentList.Add("Bypass");
        start.ArgumentList.Add("-File"); start.ArgumentList.Add(script);
        start.ArgumentList.Add(Environment.ProcessId.ToString()); start.ArgumentList.Add(payload); start.ArgumentList.Add(installRoot);
        if (Process.Start(start) is null) throw new IOException("Could not start the update installer.");
        owner.Close();
    }
}
