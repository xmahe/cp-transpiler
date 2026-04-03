# Neovim Support

This directory contains a small Neovim syntax package for `c+`.

It does three things:

- detects `.cp` and `.hp` files as `cplus`
- reuses Vim's built-in C highlighting for the C-like parts
- adds `c+`-specific highlighting for things like `fn`, `interface`, `inject`, `bind`, `maybe<T>`, and `export_c`

## Install

Point Neovim's `runtimepath` at this directory, or copy the subfolders into your local config.

Example:

```lua
vim.opt.runtimepath:append("/Users/magnus/c+/nvim")
```

Or copy:

- `nvim/ftdetect/cplus.vim` -> `~/.config/nvim/ftdetect/cplus.vim`
- `nvim/ftplugin/cplus.vim` -> `~/.config/nvim/ftplugin/cplus.vim`
- `nvim/syntax/cplus.vim` -> `~/.config/nvim/syntax/cplus.vim`

## What It Highlights

- keywords: `class`, `interface`, `namespace`, `fn`, `implements`, `implementation`, `inject`, `bind`, `export_c`
- control flow: `if`, `else`, `for`, `while`, `switch`, `case`, `default`, `return`, `break`
- builtin types: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64`, `bool`, `void`
- special type: `maybe<T>`
- lifecycle names: `Construct`, `Destruct`
- enum members in `kEnumValue` style
- `_private_like_names`
- names that follow `class`, `interface`, `enum`, `namespace`, and `fn`

## Notes

- This is Vimscript syntax highlighting, not a tree-sitter grammar.
- It should work in plain Vim too, but it is written for Neovim first.
- It aims to look good with most colorschemes by linking to standard highlight groups instead of hardcoding colors.
