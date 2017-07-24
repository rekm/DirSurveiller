set nocompatible
filetype off

set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()
Plugin 'VundleVim/Vundle.vim'
Plugin 'vim-syntastic/syntastic'
Plugin 'Valloric/YouCompleteMe'
Plugin 'tpope/vim-fugitive'
Plugin 'scrooloose/nerdtree'
call vundle#end()
filetype plugin indent on
autocmd ColorScheme * highlight ExtraWhitespace ctrembg=red guibg=red

"Fix Backspace
set backspace=indent,eol,start
"Fix Clipboard
set clipboard=unnamed

"C Mode
augroup project
    autocmd!
    autocmd BufRead,BufNewFile *.h,*.c set filetype=c.doxygen
augroup END

"Nerdtree
map <C-n> :NERDTreeToggle<CR>

syntax on
set mouse=n
set tabstop=4
set shiftwidth=4
set expandtab
set exrc
set nu

autocmd BufWritePre * %s/\\+$//e

"Style

set tw=79
set nowrap "don't wrap
set fo-=t
set colorcolumn=80
highlight ColorColumn ctermbg=gray

"Syntastic

let g:syntastic_always_populate_loc_list = 1
let g:syntastic_auto_loc_list = 1

"Make
nnoremap <F4> :make!<CR>
map <F9> :YcmCompleter FixIt<CR>
map <F3> :YcmCompleter GoTo<CR>
":set shellcmdflag=-ic
" map to project build script^
