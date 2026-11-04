import argparse
import os
import sys
import pefile

# Mapping PE machine constants to human-readable architectures
MACHINE_TYPES = {
    pefile.MACHINE_TYPE["IMAGE_FILE_MACHINE_I386"]: ("x86 (32-bit)", "x86"),
    pefile.MACHINE_TYPE["IMAGE_FILE_MACHINE_AMD64"]: ("x64 / AMD64 (64-bit)", "x64"),
    pefile.MACHINE_TYPE["IMAGE_FILE_MACHINE_ARM"]: ("ARM (32-bit)", "arm"),
    pefile.MACHINE_TYPE["IMAGE_FILE_MACHINE_ARMNT"]: ("ARM Thumb-2", "arm"),
    pefile.MACHINE_TYPE["IMAGE_FILE_MACHINE_ARM64"]: ("ARM64 / AArch64 (64-bit)", "arm64"),
    pefile.MACHINE_TYPE["IMAGE_FILE_MACHINE_IA64"]: ("Intel Itanium (IA-64)", "ia64"),
}

# Mapping PE Subsystem version numbers to Windows OS releases
OS_VERSION_MAP = {
    (3, 10): "Windows NT 3.1",
    (3, 50): "Windows NT 3.5",
    (3, 51): "Windows NT 3.51",
    (4, 0): "Windows NT 4.0 / Windows 95",
    (5, 0): "Windows 2000",
    (5, 1): "Windows XP (32-bit)",
    (5, 2): "Windows XP (64-bit) / Server 2003",
    (6, 0): "Windows Vista / Server 2008",
    (6, 1): "Windows 7 / Server 2008 R2",
    (6, 2): "Windows 8 / Server 2012",
    (6, 3): "Windows 8.1 / Server 2012 R2",
    (10, 0): "Windows 10 / Windows 11 / Server 2016+",
}

SUBSYSTEM_MAP = {
    1: "Native / Driver",
    2: "Windows GUI (Desktop / Windowed Application)",
    3: "Windows CUI (Console / Command Line Application)",
    7: "POSIX CUI",
    9: "Windows CE GUI",
    10: "EFI Application",
    14: "Xbox",
    16: "Windows Boot Application",
}


def get_min_windows_os(major: int, minor: int) -> str:
    """Resolve (Major, Minor) version into minimum Windows release."""
    if (major, minor) in OS_VERSION_MAP:
        return OS_VERSION_MAP[(major, minor)]
    if (major, minor) > (10, 0):
        return f"Windows 10 / 11 or newer (Version {major}.{minor})"
    return f"Windows NT version {major}.{minor}"


def analyze_exe(file_path: str):
    if not os.path.isfile(file_path):
        print(f"[-] Error: File '{file_path}' does not exist.")
        sys.exit(1)

    try:
        pe = pefile.PE(file_path, fast_load=True)
    except pefile.PEFormatError:
        print(f"[-] Error: '{file_path}' is not a valid Windows PE/EXE file.")
        sys.exit(1)

    # 1. Architecture
    machine_val = pe.FILE_HEADER.Machine
    arch_name, arch_code = MACHINE_TYPES.get(
        machine_val, (f"Unknown Architecture (0x{machine_val:04X})", "unknown")
    )

    # 2. Subsystem & Subsystem Version
    major_sub = getattr(pe.OPTIONAL_HEADER, "MajorSubsystemVersion", 0)
    minor_sub = getattr(pe.OPTIONAL_HEADER, "MinorSubsystemVersion", 0)
    subsystem_type = SUBSYSTEM_MAP.get(
        pe.OPTIONAL_HEADER.Subsystem, f"Other ({pe.OPTIONAL_HEADER.Subsystem})"
    )
    min_os = get_min_windows_os(major_sub, minor_sub)

    # 3. .NET CLR Detection
    com_desc_entry = pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR"]
    is_dotnet = False
    if len(pe.OPTIONAL_HEADER.DATA_DIRECTORY) > com_desc_entry:
        is_dotnet = pe.OPTIONAL_HEADER.DATA_DIRECTORY[com_desc_entry].VirtualAddress != 0

    # 4. Generate Compatibility Summary
    compatibility_notes = []

    if is_dotnet:
        compatibility_notes.append(
            "- [.NET Managed Binary]: Runs on any Windows OS with the required .NET Runtime installed."
        )

    if arch_code == "x86":
        compatibility_notes.extend([
            f"- 32-bit Windows: Runs natively on {min_os} and newer (up to 32-bit Windows 10).",
            f"- 64-bit Windows: Runs via WOW64 emulation on 64-bit Windows XP through Windows 11.",
            "- Windows on ARM: Runs via x86 emulation on Windows 10/11 ARM64 devices.",
        ])
    elif arch_code == "x64":
        compatibility_notes.extend([
            "- 32-bit Windows: INCOMPATIBLE (Will NOT run on any 32-bit Windows OS).",
            f"- 64-bit Windows: Runs natively on 64-bit {min_os} through Windows 11.",
            "- Windows on ARM: Runs on Windows 11 ARM64 (via x64 emulation). Not supported on older Windows 10 ARM versions.",
        ])
    elif arch_code == "arm64":
        compatibility_notes.extend([
            "- Standard x86/x64 Windows: INCOMPATIBLE.",
            f"- Windows on ARM: Runs natively on Windows 10/11 ARM64 (e.g., Surface Pro X, Snapdragon X Elite laptops).",
        ])
    elif arch_code == "arm":
        compatibility_notes.append(
            "- Windows RT / Windows 10 Mobile / Windows IoT on 32-bit ARM devices only."
        )

    # Output Results
    print("=" * 60)
    print(f" PE Analysis Report: {os.path.basename(file_path)}")
    print("=" * 60)
    print(f"Target Architecture : {arch_name}")
    print(f"Subsystem           : {subsystem_type}")
    print(f"Minimum OS Version  : {min_os} (PE Subsystem version {major_sub}.{minor_sub})")
    print(f"Managed (.NET) App  : {'Yes (.NET Assembly)' if is_dotnet else 'No (Native Binary)'}")
    print("-" * 60)
    print("Execution Compatibility:")
    for note in compatibility_notes:
        print(f"  {note}")
    print("=" * 60)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Inspect a Windows .exe for OS and architecture compatibility.")
    parser.add_argument("exe_path", help="Path to the executable file (.exe)")
    args = parser.parse_args()

    analyze_exe(args.exe_path)