import os
import argparse

# Define characters for drawing the tree
# You can customize these if you like
TREE_BRANCH = "├── "
TREE_LAST_BRANCH = "└── "
TREE_VERTICAL = "│   "
TREE_SPACE = "    "

def print_directory_tree(
    root_dir,
    prefix="",
    level=-1,
    limit_to_level=-1,
    ignore_list=None,
    show_hidden=False,
    include_files=True,
    only_dirs=False
):
    """
    Prints a directory tree structure.

    Args:
        root_dir (str): The path to the root directory to scan.
        prefix (str): The prefix string for the current line (used for indentation).
        level (int): Current depth level (for internal recursion).
        limit_to_level (int): Maximum depth to print (-1 for no limit).
        ignore_list (list): A list of directory or file names to ignore.
        show_hidden (bool): Whether to show hidden files/directories (starting with '.').
        include_files (bool): Whether to include files in the output.
        only_dirs (bool): If True, only show directories.
    """
    if ignore_list is None:
        ignore_list = []

    if level > -1 and limit_to_level > -1 and level >= limit_to_level:
        return

    try:
        # Get all items in the directory, handling potential permission errors
        items = sorted(os.listdir(root_dir))
    except PermissionError:
        print(f"{prefix}{TREE_BRANCH}[Access Denied: {os.path.basename(root_dir)}]")
        return
    except FileNotFoundError:
        print(f"Error: Directory not found - {root_dir}")
        return


    # Filter items
    filtered_items = []
    for item_name in items:
        if item_name in ignore_list:
            continue
        if not show_hidden and item_name.startswith('.'):
            continue
        full_path = os.path.join(root_dir, item_name)
        if only_dirs and not os.path.isdir(full_path):
            continue
        if not include_files and os.path.isfile(full_path):
            continue
        filtered_items.append(item_name)

    pointers = [TREE_BRANCH] * (len(filtered_items) - 1) + [TREE_LAST_BRANCH]

    for pointer, item_name in zip(pointers, filtered_items):
        full_path = os.path.join(root_dir, item_name)
        is_dir = os.path.isdir(full_path)

        print(f"{prefix}{pointer}{item_name}{'/' if is_dir else ''}")

        if is_dir:
            extension = TREE_VERTICAL if pointer == TREE_BRANCH else TREE_SPACE
            print_directory_tree(
                full_path,
                prefix + extension,
                level + 1,
                limit_to_level,
                ignore_list,
                show_hidden,
                include_files,
                only_dirs
            )

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Display a directory tree structure.")
    parser.add_argument(
        "directory",
        nargs="?",
        default=".",
        help="The directory to scan (default: current directory)."
    )
    parser.add_argument(
        "-L", "--level",
        type=int,
        default=-1,
        help="Descend only level directories deep."
    )
    parser.add_argument(
        "-I", "--ignore",
        action="append",
        default=[],
        help="Do not list files/directories that match the given pattern(s). Can be used multiple times."
             " Example: -I __pycache__ -I .git"
    )
    parser.add_argument(
        "-a", "--all",
        action="store_true",
        help="All files are listed (shows hidden files/directories)."
    )
    parser.add_argument(
        "-d", "--dirs-only",
        action="store_true",
        help="List directories only."
    )
    parser.add_argument(
        "--no-files",
        action="store_true",
        help="Do not list files (effectively same as --dirs-only if no other file-like entries)."
    )


    args = parser.parse_args()

    root_dir_path = os.path.abspath(args.directory)
    print(f"{os.path.basename(root_dir_path)}/") # Print the root directory name

    # Default ignore list
    default_ignores = ['.git', '__pycache__', '.vscode', '.idea', 'node_modules', 'build', 'dist']
    combined_ignore_list = list(set(default_ignores + args.ignore))


    print_directory_tree(
        root_dir_path,
        prefix="",
        level=0, # Start at level 0 for the first set of items
        limit_to_level=args.level,
        ignore_list=combined_ignore_list,
        show_hidden=args.all,
        include_files=not args.no_files and not args.dirs_only,
        only_dirs=args.dirs_only
    )