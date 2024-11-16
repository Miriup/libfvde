"
" This file adds folding for libfvde log files to vim.
"
" It's quite slow, but once it's done its thing you will be fast.
"
" To use it:
" :source documentation/libfvde.vim
" :set foldmethod=expr
" :set foldexpr=_libfvde_expr()
" :set foldtext=_libfvde_text()

" folding levels
" 0 = volume
" 1 = metadata
" 2 = metadata type
" 3 = plist
" +1  binary dumps
"
function! _libfvde_expr()
	let line=getline(v:lnum)
	if line[0:17] == 'Reading metadata: ' || line[0:26] == 'Reading encrypted metadata '
		return '>1'
	endif
	if line[0:33] == 'libfvde_metadata_block_read_data: '
"		if line[34:] == 'header data:'
"			return '>2'
"		endif
		return 2
	endif
	" Metadata blocks
	if line[0:28]=='libfvde_metadata_read_type_0x'
		if line[35:47] == 'metadata size'
			return '>2'
		endif
		return 2
	endif
	if line[0:38] == 'libfvde_encrypted_metadata_read_type_0x'
		return 2
	endif
	" Sub metadata block level dumps
	if line[0:41] == 'libfvde_metadata_read_volume_group_plist: ' 
		if line[42:] == 'XML:'
			return '>3'
		endif
		return 3
	endif
	if line[0:13] == 'libfplist_xml_'
		return 4
	endif
	if line[0:52] == 'libfvde_encrypted_metadata_read_from_file_io_handle: '
		return 1
	endif
	" Data dumps
	if line[0:9]=='00000000: '
		return 'a1'
	endif
	if line=~'^[0-9a-f]\{8}: '
		return '='
	endif
	" Omitted data
	if line=='...'
		return '='
	endif
	" Dump end
	if line==''
		" Handle binary dump ends explicitely for convenience
		if getline(v:lnum-1)=~'^[0-9a-f]\{8}: '
			return 's1'
		endif
		return -1
	endif
	return '='
endfunction

let s:_libfvde_blockdesc = {
	\'0x0011': 'Volume group info',
	\'0x0010': 'information about the physical volume header',
	\'0x0012': 'Volume group info',
	\'0x0013': 'information about a transaction',
	\'0x0014': 'information about a transaction',
	\'0x0016': '?',
	\'0x0017': '?',
	\'0x0018': '?',
	\'0x0019': 'information about the encryption context of the logical volume',
	\'0x001a': 'information about the logical volume',
	\'0x001c': '?',
	\'0x001d': 'information about unused areas of a physical volume',
	\'0x0021': '?',
	\'0x0022': 'information about unused areas of a physical volume',
	\'0x0024': 'used to store data that does not fit within a single metadata block',
	\'0x0025': '?',
	\'0x0105': 'information about logical volumes',
	\'0x0205': '?',
	\'0x0303': '? not documented',
	\'0x0304': '?',
	\'0x0305': 'information about location of the logical volume data inside the physical volumes',
	\'0x0403': '? not documented',
	\'0x0404': 'information about data and metadata areas of the physical volume',
	\'0x0405': 'information about data and metadata areas of a physical volume',
	\'0x0505': 'information about location of the logical volume data inside the physical volumes',
	\'0x0605': '?',
	\'?'	 : '?'
	\}

function! _libfvde_text()
	" Number of lines in the fold
	let nl = v:foldend - v:foldstart + 1
	let block_type = '?'
	let transaction = '?'
	let number = '?'
	let signature = ''
	" Check the first 10 lines for known output
	for offset in [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]
		if offset >= nl
			break
		endif
		let line = getline(v:foldstart+offset)
		if line[0:43] == 'libfvde_metadata_block_read_data: signature	'
			let signature = line[49:]
		endif
		if line[0:38] == 'libfvde_metadata_block_read_data: type	'
			let block_type = line[45:50]
		endif
		if line[0:56] == 'libfvde_metadata_block_read_data: transaction identifier	'
			let transaction = line[60:]
		endif
		if line[0:51] == 'libfvde_metadata_block_read_data: object identifier	'
			let object = line[56:]
		endif
		if line[0:40] == 'libfvde_metadata_block_read_data: number	'
			let num = line[46:]
			return v:folddashes . nl . " lines: ".signature." metadata block " . num . " transaction " . transaction . " object " . object . " read " . block_type . " (" . s:_libfvde_blockdesc[block_type] . ")"
		endif
	endfor
	return foldtext()
endfunction
