if exists("b:current_syntax")
  finish
endif

runtime! syntax/c.vim
unlet! b:current_syntax

syntax keyword cplusKeyword class interface namespace fn implements implementation inject bind export_c public private static
syntax keyword cplusConditional if else switch case default
syntax keyword cplusRepeat for while
syntax keyword cplusStatement return break
syntax keyword cplusBoolean true false null
syntax keyword cplusBuiltinType void bool u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 usize isize
syntax keyword cplusLifecycle Construct Destruct
syntax keyword cplusReserved continue goto
syntax keyword cplusSelf self

syntax match cplusArrow "->"
syntax match cplusScope "::"
syntax match cplusMaybe "\<maybe\ze\s*<"
syntax match cplusEnumMember "\<k[A-Z][A-Za-z0-9_]*\>"
syntax match cplusPrivateName "\<_\l[A-Za-z0-9_]*\>"
syntax match cplusTypeDecl "\<[A-Z][A-Za-z0-9_]*\%(\s*::\s*[A-Z][A-Za-z0-9_]*\)*\>" contained
syntax match cplusFunctionDecl "\<[A-Za-z_][A-Za-z0-9_]*\%(\s*::\s*[A-Za-z_][A-Za-z0-9_]*\)*\ze\s*(" contained

syntax keyword cplusDeclKeyword class interface enum namespace nextgroup=cplusTypeDecl skipwhite
syntax keyword cplusFnKeyword fn nextgroup=cplusFunctionDecl skipwhite

highlight default link cplusKeyword Keyword
highlight default link cplusDeclKeyword Keyword
highlight default link cplusFnKeyword Keyword
highlight default link cplusConditional Conditional
highlight default link cplusRepeat Repeat
highlight default link cplusStatement Statement
highlight default link cplusBoolean Boolean
highlight default link cplusBuiltinType Type
highlight default link cplusLifecycle Special
highlight default link cplusReserved Error
highlight default link cplusSelf Special
highlight default link cplusArrow Operator
highlight default link cplusScope Operator
highlight default link cplusMaybe Type
highlight default link cplusEnumMember Constant
highlight default link cplusPrivateName Identifier
highlight default link cplusTypeDecl Type
highlight default link cplusFunctionDecl Function

let b:current_syntax = "cplus"
