#pragma once

// Copied from eq1's plugin/PresetLibrary.h (eq1 at dbb1127), in namespace plugkit::ui, its product name, Preset file
// extension, root type and Factory Presets from a Config, where eq1's are "eq1", ".eq1preset", "eq1" and its
// FactoryPresets binary data: so fileExtension(), read() and factoryPresets() belong to a library, and
// defaultUserFolder() takes the product name.

#include <juce_audio_processors/juce_audio_processors.h>

#include <optional>
#include <vector>

namespace plugkit::ui
{

// Where Presets are kept: User Presets as files in a folder and its subfolders, one per Preset, named
// after it; Factory Presets bundled with the plugin. A Preset file is plain XML in the saved state's format
// (Session::presetState), so it moves between machines and opens in newer versions.
class PresetLibrary
{
public:
    // A Factory Preset as the plugin bundles it (its juce_add_binary_data): its name, and its file's bytes.
    struct FactoryPresetData
    {
        juce::String name;
        const char* data = nullptr;
        int size = 0;
    };
    // What the plugin's library is.
    struct Config
    {
        juce::String productName;   // the User folder is Documents/<productName>/Presets
        juce::String fileExtension; // a Preset file's, with its dot: ".eq1preset"
        juce::Identifier rootType;  // a Preset's root type: the saved state's type
        std::vector<FactoryPresetData> factoryPresets;
    };

    // Its User Presets kept in userFolder, or defaultUserFolder (config.productName) when that is empty.
    explicit PresetLibrary (Config config, juce::File userFolder = {});

    // Documents/<productName>/Presets.
    static juce::File defaultUserFolder (const juce::String& productName);
    // The User folder this library keeps its Presets in.
    const juce::File& folder() const { return userFolder; }
    const juce::String& fileExtension() const { return config.fileExtension; }
    // The User folder itself as Entry::folder names it; its subfolders' paths start with it and a /.
    static inline const juce::String userFolderName { "User" };

    // The User Presets' files, in browser order: the User folder's own, then its subfolders' at any
    // depth, depth-first, each level by name. None when the folder doesn't exist yet.
    std::vector<juce::File> userPresets() const;
    // The User folder's subfolders at any depth, holding Presets or not, in browser order, by their
    // paths as Entry::folder names them ("User/Drums"). None when the folder doesn't exist yet.
    std::vector<juce::String> userSubfolders() const;

    // The name a Preset is saved under for name: trimmed, with the characters a file name can't hold
    // taken out. Empty when nothing is left, which save() refuses, as the editor refuses an empty
    // name, instead of writing a hidden file named by the extension alone.
    static juce::String legalName (const juce::String& name);
    // Saves a Preset as legalName (name) in the User folder, creating the folder if need be and
    // replacing a Preset of the same name. Returns the file, or nothing when the name has no legal
    // characters or the file can't be written.
    std::optional<juce::File> save (const juce::String& name, const juce::ValueTree& preset) const;

    // A Preset file's contents, or an invalid tree when it isn't one (not XML, or not of the root type).
    juce::ValueTree read (const juce::File& file) const;

    struct FactoryPreset
    {
        juce::String name;
        juce::ValueTree preset;
    };
    // The Factory Presets, by name. One whose data isn't XML holds an invalid tree.
    std::vector<FactoryPreset> factoryPresets() const;

    // A Preset as the browser lists it.
    struct Entry
    {
        juce::String name;
        // "Factory", "User", or a User subfolder's path, such as "User/Drums/Acoustic".
        juce::String folder;
        // A User Preset's file; none for a Factory Preset.
        juce::File file;
        juce::ValueTree preset;
    };
    // Every Preset, read from the disk now, in browser order: Factory, then User (userPresets()). A
    // User Preset file that doesn't read as a Preset is left out.
    std::vector<Entry> listing() const;
    // A folder as the browser's folder column shows it.
    struct Folder
    {
        juce::String path; // as Entry::folder
        juce::String name; // its last part: "Factory", "User", "Acoustic"
        int depth = 0;     // 0 for Factory and User, 1 for User's subfolders, and so on
        int count = 0;     // the Presets directly in it
    };
    // Factory, User and each User subfolder, in browser order, a subfolder after the folder it is in:
    // those in subfolders (userSubfolders()), and those the entries' Presets are in.
    static std::vector<Folder> folders (const std::vector<Entry>& entries, const std::vector<juce::String>& subfolders = {});
    // The entries whose Preset names contain text, ignoring case, in their order. All of them for no text.
    static std::vector<Entry> search (const std::vector<Entry>& entries, const juce::String& text);
    // The Loaded Preset's entry: lastLoaded's place (folder and name) when it is listed and still named
    // loadedPreset, as a name can be listed more than once; otherwise the first entry named loadedPreset.
    // Nothing when none is.
    static std::optional<std::size_t> find (const std::vector<Entry>& entries, const juce::String& loadedPreset, const Entry* lastLoaded = nullptr);
    // The entry by places after (or before, when negative) the Loaded Preset's (find), wrapping at the
    // ends. With no such entry, forward is the first and back the last. Nothing when there are none.
    static std::optional<std::size_t>
        step (const std::vector<Entry>& entries, const juce::String& loadedPreset, int by, const Entry* lastLoaded = nullptr);

private:
    Config config;
    juce::File userFolder;
};

} // namespace plugkit::ui
