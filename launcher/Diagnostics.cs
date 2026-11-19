using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace Wwe2k16Launcher;

// One text report with everything needed to help a player who cannot say why the game does not start: versions, settings,
// the installed title update (checked against the one the game is built for), the graphics card the game found, and the
// newest game log (its warnings and errors, and its end). "Copy diagnostic" in the launcher puts it on the clipboard.
internal static class Diagnostics
{
    // The title update the game is built for: WWE 2K16 TU3, default.xexp that patches the game to version 0.0.5.3. Another
    // revision (e.g. 0.0.3.3, seen on some download sites) makes the game stop right after the start.
    public const string ExpectedXexpSha256 = "8FDCF1EE8420951D3039555F6BC179A37B3C5B9F31952625B9B47FBEA4425BCC";
    const long MaxReport = 80_000;

    public static string? NewestLog(LauncherPaths paths)
    {
        try
        {
            if (!Directory.Exists(paths.Logs)) return null;
            return new DirectoryInfo(paths.Logs).GetFiles("run-*.log").OrderByDescending(f => f.LastWriteTimeUtc)
                .FirstOrDefault()?.FullName;
        }
        catch (IOException) { return null; }
    }

    // null = no title update installed or unreadable; true = the expected revision; false = another file.
    public static bool? TitleUpdateIsExpected(TitleUpdate tu, out string sha, out long size)
    {
        sha = ""; size = 0;
        string file = Path.Combine(tu.Folder, "default.xexp");
        try
        {
            if (!File.Exists(file)) return null;
            size = new FileInfo(file).Length;
            using var stream = File.OpenRead(file);
            sha = Convert.ToHexString(SHA256.HashData(stream));
            return sha.Equals(ExpectedXexpSha256, StringComparison.OrdinalIgnoreCase);
        }
        catch (IOException) { return null; }
        catch (UnauthorizedAccessException) { return null; }
    }

    // A short, plain explanation of why the newest log ended early, or null when nothing in it is recognised.
    public static string? ExplainLog(string? logPath)
    {
        if (logPath is null) return null;
        try
        {
            string text = ReadShared(logPath);
            if (text.Contains("No function registered at 822427E0", StringComparison.OrdinalIgnoreCase))
                return "Your title update is not the right revision (the game is built for the one that patches it to version 0.0.5.3). Install the correct Title Update 3.";
            if (text.Contains("could not create the host device", StringComparison.OrdinalIgnoreCase))
                return "Your graphics card or driver does not support Vulkan 1.2. Update the graphics driver.";
            if (text.Contains("No function registered at", StringComparison.OrdinalIgnoreCase))
                return "The game files do not match the title update. Check your game copy and Title Update 3.";
        }
        catch (IOException) { }
        return null;
    }

    public static string Build(LauncherPaths paths, LauncherSettings settings, TitleUpdate tu, string? gameFolderKind)
    {
        var sb = new StringBuilder();
        string home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        // The user's profile folder holds their Windows user name: hidden, also in its JSON-escaped form (C:\Users\name).
        string Hide(string s) => string.IsNullOrEmpty(home) ? s
            : s.Replace(home, "%USERPROFILE%", StringComparison.OrdinalIgnoreCase)
               .Replace(home.Replace("\\", "\\\\"), "%USERPROFILE%", StringComparison.OrdinalIgnoreCase);

        sb.AppendLine("WWE 2K16 PC diagnostic");
        sb.AppendLine($"Launcher {AppUpdater.CurrentVersion} | {Environment.OSVersion.VersionString} | {Environment.ProcessorCount} logical CPUs | {(Environment.Is64BitOperatingSystem ? "64-bit" : "32-bit")}");
        sb.AppendLine($"Created {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
        sb.AppendLine();

        sb.AppendLine("== Game and title update ==");
        sb.AppendLine($"Game: {gameFolderKind ?? "unknown"}");
        sb.AppendLine($"Game exe present: {File.Exists(paths.TitleUpdateExe)} | install folder: {Hide(paths.Root)}");
        bool? ok = TitleUpdateIsExpected(tu, out string sha, out long size);
        sb.AppendLine(ok switch
        {
            null => "Title update: not installed",
            true => $"Title update: OK (the expected revision, {size} bytes)",
            _ => $"Title update: DIFFERENT from the expected revision ({size} bytes, SHA-256 {sha}); expected {ExpectedXexpSha256}",
        });
        sb.AppendLine();

        sb.AppendLine("== Launcher settings ==");
        try
        {
            string json = JsonSerializer.Serialize(settings, new JsonSerializerOptions { WriteIndented = true });
            sb.AppendLine(Hide(json));
        }
        catch (NotSupportedException) { }
        sb.AppendLine();

        string? log = NewestLog(paths);
        sb.AppendLine($"== Newest game log: {(log is null ? "none (the game has not been started from this launcher folder)" : Path.GetFileName(log))} ==");
        if (log is not null)
        {
            try
            {
                string[] lines = ReadShared(log).Split('\n').Select(l => l.TrimEnd('\r')).ToArray();
                sb.AppendLine($"{lines.Length} lines");
                string? explain = ExplainLog(log);
                if (explain is not null) sb.AppendLine("Probable cause: " + explain);
                sb.AppendLine();
                sb.AppendLine("-- start of the log (first 45 lines) --");
                foreach (var l in lines.Take(45)) sb.AppendLine(Hide(l));
                var problems = lines.Skip(45).Where(l => l.Contains("[error]") || l.Contains("[critical]")).Distinct().Take(25).ToArray();
                if (problems.Length > 0)
                {
                    sb.AppendLine();
                    sb.AppendLine("-- errors --");
                    foreach (var l in problems) sb.AppendLine(Hide(l));
                }
                sb.AppendLine();
                sb.AppendLine("-- end of the log (last 40 lines) --");
                foreach (var l in lines.Skip(Math.Max(45, lines.Length - 40))) sb.AppendLine(Hide(l));
            }
            catch (IOException ex) { sb.AppendLine("The log could not be read: " + ex.Message); }
        }

        string report = sb.ToString();
        return report.Length > MaxReport ? report[..(int)MaxReport] + "\n[cut]\n" : report;
    }

    static string ReadShared(string path)
    {
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        using var reader = new StreamReader(stream);
        return reader.ReadToEnd();
    }
}
