namespace Wwe2k16Launcher;

// Titles of the Windows file and folder dialogs in the launcher's language (the page translates everything else).
// Keyed by the English text; languages are the game's Language setting values.
static class DialogText
{
    // [English, French, German, Italian, Spanish]
    static readonly string[][] Rows =
    [
        ["Choose your WWE 2K16 disc image", "Choisissez votre image disque de WWE 2K16", "Wähle dein WWE-2K16-Disc-Image", "Scegli la tua immagine disco di WWE 2K16", "Elige tu imagen de disco de WWE 2K16"],
        ["Choose the folder of your extracted WWE 2K16 disc (the one containing default.xex)", "Choisissez le dossier de votre disque WWE 2K16 extrait (celui qui contient default.xex)", "Wähle den Ordner deiner entpackten WWE-2K16-Disc (der mit default.xex)", "Scegli la cartella del tuo disco WWE 2K16 estratto (quella con default.xex)", "Elige la carpeta de tu disco de WWE 2K16 extraído (la que contiene default.xex)"],
        ["Add WWE 2K16 DLC packages", "Ajouter des paquets DLC de WWE 2K16", "WWE-2K16-DLC-Pakete hinzufügen", "Aggiungi pacchetti DLC di WWE 2K16", "Añadir paquetes DLC de WWE 2K16"],
        ["Choose a folder with WWE 2K16 DLC packages (subfolders are searched too)", "Choisissez un dossier contenant des paquets DLC de WWE 2K16 (sous-dossiers compris)", "Wähle einen Ordner mit WWE-2K16-DLC-Paketen (Unterordner werden mit durchsucht)", "Scegli una cartella con pacchetti DLC di WWE 2K16 (anche nelle sottocartelle)", "Elige una carpeta con paquetes DLC de WWE 2K16 (también se buscan subcarpetas)"],
        ["Choose the WWE 2K16 title update file (e.g. tu00000003_00000000)", "Choisissez le fichier de mise à jour de WWE 2K16 (ex. tu00000003_00000000)", "Wähle die WWE-2K16-Update-Datei (z. B. tu00000003_00000000)", "Scegli il file di aggiornamento di WWE 2K16 (es. tu00000003_00000000)", "Elige el archivo de actualización de WWE 2K16 (p. ej. tu00000003_00000000)"],
        ["Add custom logos", "Ajouter des logos perso", "Eigene Logos hinzufügen", "Aggiungi loghi personalizzati", "Añadir logotipos personalizados"],
        ["Add songs to the WWE 2K16 custom soundtrack", "Ajouter des morceaux à la bande-son perso de WWE 2K16", "Songs zum eigenen WWE-2K16-Soundtrack hinzufügen", "Aggiungi brani alla colonna sonora personalizzata di WWE 2K16", "Añadir canciones a la banda sonora personalizada de WWE 2K16"],
        ["Select mod package (.zip) or asset files (.pac, .bik)", "Choisissez un paquet de mod (.zip) ou des fichiers (.pac, .bik)", "Mod-Paket (.zip) oder Dateien (.pac, .bik) auswählen", "Scegli un pacchetto mod (.zip) o dei file (.pac, .bik)", "Elige un paquete de mod (.zip) o archivos (.pac, .bik)"],
        ["Select an extracted mod folder (containing pac, stream, or asset files)", "Choisissez un dossier de mod extrait (avec des fichiers pac, stream ou autres)", "Entpackten Mod-Ordner auswählen (mit pac-, stream- oder anderen Dateien)", "Scegli una cartella mod estratta (con file pac, stream o altri)", "Elige una carpeta de mod extraída (con archivos pac, stream u otros)"],
        ["Export control scheme", "Exporter le schéma de commandes", "Steuerungsschema exportieren", "Esporta schema dei comandi", "Exportar esquema de controles"],
        ["Import control scheme", "Importer un schéma de commandes", "Steuerungsschema importieren", "Importa schema dei comandi", "Importar esquema de controles"],
    ];

    static readonly Dictionary<string, int> Columns = new()
    {
        ["french"] = 1, ["german"] = 2, ["italian"] = 3, ["spanish"] = 4,
    };

    public static string Get(string? language, string english)
    {
        if (!Columns.TryGetValue((language ?? "").ToLowerInvariant(), out int column)) return english;
        foreach (var row in Rows)
            if (row[0] == english) return row[column];
        return english;
    }
}
