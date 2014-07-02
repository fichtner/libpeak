/*
 * Copyright (c) 2014 Tobias Boertitz <tobias@packetwerk.com>
 * Copyright (c) 2014 Franco Fichtner <franco@packetwerk.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef PEAK_MAGIC_H
#define PEAK_MAGIC_H

/*
 * Begin of autogenerated section.  DO NOT EDIT.
 */

struct magic_item {
	unsigned int guid;
	const char *description;
	const char *mime_type;
};

enum {
	MAGIC_UNKNOWN = 0U,
	MAGIC_DICOM_MEDICAL_IMAGING_DATA,
	MAGIC_NODE_JS_SCRIPT_TEXT_EXECUTABLE,
	MAGIC_BINHEX_BINARY_TEXT,
	MAGIC_MARC21,
	MAGIC_MICROSOFT_WORD_DOCUMENT,
	MAGIC_PACKED_DATA,
	MAGIC_OGG_DATA,
	MAGIC_PDF_DOCUMENT,
	MAGIC_PGP_MESSAGE,
	MAGIC_PGP_PUBLIC_KEY_BLOCK,
	MAGIC_PGP_SIGNATURE,
	MAGIC_POSTSCRIPT_DOCUMENT_TEXT,
	MAGIC_SEREAL_DATA,
	MAGIC_PART_OF_MULTIPART_DEBIAN_PACKAGE,
	MAGIC_FDF_DOCUMENT,
	MAGIC_SPLINE_FONT_DATABASE,
	MAGIC_GOOGLE_KML_DOCUMENT,
	MAGIC_COMPRESSED_GOOGLE_KML_DOCUMENT__INCLUDING_RESOURCES_,
	MAGIC_SUN_KCMS_ICC_PROFILE,
	MAGIC_LOTUS_WORDPRO,
	MAGIC_MICROSOFT_CABINET_ARCHIVE_DATA,
	MAGIC_MICROSOFT_EXCEL_WORKSHEET,
	MAGIC_EMBEDDED_OPENTYPE__EOT_,
	MAGIC_OPENTYPE_FONT_DATA,
	MAGIC_TRANSPORT_NEUTRAL_ENCAPSULATION_FORMAT,
	MAGIC_MICROSOFT_WORD_2007P,
	MAGIC_REALMEDIA_FILE,
	MAGIC_SYMBIAN_INSTALLATION_FILE,
	MAGIC_TCPDUMP_CAPTURE_FILE,
	MAGIC_LOTUS_1_2_3,
	MAGIC_ABOOK_ADDRESS_BOOK,
	MAGIC_ARC_ARCHIVE_DATA,
	MAGIC_MIPS_ARCHIVE,
	MAGIC_ARJ_ARCHIVE_DATA,
	MAGIC_BITTORRENT_FILE,
	MAGIC_BZIP2_COMPRESSED_DATA,
	MAGIC_COMPRESS_D_DATA,
	MAGIC_BYTE_SWAPPED_CPIO_ARCHIVE,
	MAGIC_BERKELEY_DB,
	MAGIC_WINDOWS_PROGRAM_INFORMATION_FILE,
	MAGIC_TEX_DVI_FILE,
	MAGIC_EET_ARCHIVE,
	MAGIC_EMACS__BYTE_COMPILED_LISP_DATA,
	MAGIC_X11_SNF_FONT_DATA__LSB_FIRST,
	MAGIC_TRUETYPE_FONT_DATA,
	MAGIC_FREEMIND_DOCUMENT,
	MAGIC_FREEPLANE_DOCUMENT,
	MAGIC_GNU_DBM_OR_NDBM_DATABASE,
	MAGIC_GNUMERIC_SPREADSHEET,
	MAGIC_GPG_KEY_PUBLIC_RING,
	MAGIC_GZIP_COMPRESSED_DATA,
	MAGIC_HIERARCHICAL_DATA_FORMAT_DATA,
	MAGIC_HANGUL__KOREAN__WORD_PROCESSOR_FILE_2000,
	MAGIC_INTERNET_ARCHIVE_FILE,
	MAGIC_JUST_SYSTEM_WORD_PROCESSOR_ICHITARO,
	MAGIC_JUST_SYSTEM_WORD_PROCESSOR_ICHITARO_V5,
	MAGIC_JUST_SYSTEM_WORD_PROCESSOR_ICHITARO_V6,
	MAGIC_ISO_9660_CD_ROM_FILESYSTEM_DATA__RAW_2352_BYTE_SECTORS_,
	MAGIC_JAVA_APPLET,
	MAGIC_JAVA_JCE_KEYSTORE,
	MAGIC_JAVA_KEYSTORE,
	MAGIC_KDE_FILE,
	MAGIC_LHA_ARCHIVE_DATA,
	MAGIC_LHARC_1_X_ARX_ARCHIVE_DATA__LH0_,
	MAGIC_LZ4_COMPRESSED_DATA__V0_1_V0_9_,
	MAGIC_LZIP_COMPRESSED_DATA,
	MAGIC_LZMA_COMPRESSED_DATA_,
	MAGIC_FRAMEMAKER_FILE,
	MAGIC_MICROSOFT_ACCESS_DATABASE,
	MAGIC_PGP_KEY_RING,
	MAGIC_WINDOWS_PRECOMPILED_INF,
	MAGIC_MOTOROLA_QUARK_EXPRESS_DOCUMENT__ENGLISH_,
	MAGIC_APPLE_QUICKTIME_COMPRESSED_ARCHIVE,
	MAGIC_RAR_ARCHIVE_DATA_,
	MAGIC_RPM,
	MAGIC_SC_SPREADSHEET_FILE,
	MAGIC_SCRIBUS_DOCUMENT,
	MAGIC_SNAPPY_FRAMED_DATA,
	MAGIC_STUFFIT_ARCHIVE,
	MAGIC_PKG_DATASTREAM__SVR4_,
	MAGIC_GNU_TAR_ARCHIVE,
	MAGIC_TEX_FONT_METRIC_DATA,
	MAGIC_INITIALIZATION_CONFIGURATION,
	MAGIC_XZ_COMPRESSED_DATA,
	MAGIC_ZOO_ARCHIVE_DATA,
	MAGIC_XML_DOCUMENT_TEXT,
	MAGIC_XML_SITEMAP_DOCUMENT_TEXT,
	MAGIC_ZIP_MULTI_VOLUME_ARCHIVE_DATA__AT_LEAST_PKZIP_V2_50_TO_EXTRACT,
	MAGIC_STANDARD_MIDI_DATA,
	MAGIC_MPEG_ADTS,
	MAGIC_ATSC_A_52_AKA_AC_3_AKA_DOLBY_DIGITAL_STREAM_,
	MAGIC_MONKEY_S_AUDIO_COMPRESSED_FORMAT,
	MAGIC_FLAC_AUDIO_BITSTREAM_DATA,
	MAGIC_MPEG_ADIF__AAC,
	MAGIC_MPEG_ADTS__AAC,
	MAGIC_MODULE_SOUND_DATA,
	MAGIC_MPEG_4_LOAS,
	MAGIC_MUSEPACK_AUDIO,
	MAGIC_REALAUDIO_SOUND_FILE,
	MAGIC_UNKNOWN_AUDIO_FORMAT,
	MAGIC_MBWF_RF64_AUDIO,
	MAGIC_PROTEIN_DATA_BANK_DATA__ID_CODE__S,
	MAGIC_GIF_IMAGE_DATA,
	MAGIC_JPEG_2000_IMAGE,
	MAGIC_JPEG_IMAGE_DATA,
	MAGIC_PNG_IMAGE_DATA,
	MAGIC_SVG_SCALABLE_VECTOR_GRAPHICS_IMAGE,
	MAGIC_TIFF_IMAGE_DATA,
	MAGIC_ADOBE_PHOTOSHOP_IMAGE,
	MAGIC_DJVU_MULTIPLE_PAGE_DOCUMENT,
	MAGIC_DWG_AUTODESK_AUTOCAD_RELEASE_1_40,
	MAGIC_AWARD_BIOS_LOGO__136_X_126,
	MAGIC_AWARD_BIOS_BITMAP,
	MAGIC_CANON_CR2_RAW_IMAGE_DATA,
	MAGIC_CANON_CIFF_RAW_IMAGE_DATA,
	MAGIC_CARTESIAN_PERCEPTUAL_COMPRESSION_IMAGE,
	MAGIC_MS_WINDOWS_CURSOR_RESOURCE,
	MAGIC_DPX_IMAGE_DATA__BIG_ENDIAN_,
	MAGIC_OPENEXR_IMAGE_DATA_,
	MAGIC_MS_WINDOWS_ICON_RESOURCE,
	MAGIC_SYSLINUX__LSS16_IMAGE_DATA,
	MAGIC_PC_BITMAP__OS_2_1_X_FORMAT,
	MAGIC_NIFF_IMAGE_DATA,
	MAGIC_OLYMPUS_ORF_RAW_IMAGE_DATA__BIG_ENDIAN,
	MAGIC_PAINT_NET_IMAGE_DATA,
	MAGIC_PCX,
	MAGIC_PROGRESSIVE_GRAPHICS_IMAGE_DATA_,
	MAGIC_POLAR_MONITOR_BITMAP_TEXT,
	MAGIC_NETPBM_PPM_IMAGE_FILE,
	MAGIC_APPLE_QUICKTIME_IMAGE__FAST_START_,
	MAGIC_UNKNOWN_IMAGE_FORMAT,
	MAGIC_FOVEON_X3F_RAW_IMAGE_DATA,
	MAGIC_GIMP_XCF_IMAGE_DATA_,
	MAGIC_XCURSOR_DATA,
	MAGIC_X_PIXMAP_IMAGE_TEXT,
	MAGIC_XWD_X_WINDOW_DUMP_IMAGE_DATA,
	MAGIC_NEWS_TEXT,
	MAGIC_MAIL_TEXT,
	MAGIC_VRML_FILE,
	MAGIC_X3D__EXTENSIBLE_3D__MODEL_XML_TEXT,
	MAGIC_VCALENDAR_CALENDAR_FILE,
	MAGIC_HTML_DOCUMENT_TEXT,
	MAGIC_PGP_ENCRYPTED_DATA,
	MAGIC_RICH_TEXT_FORMAT_DATA_,
	MAGIC_TEXMACS_DOCUMENT_TEXT,
	MAGIC_TROFF_OR_PREPROCESSOR_INPUT_TEXT,
	MAGIC_ASSEMBLER_SOURCE_TEXT,
	MAGIC_AWK_SCRIPT_TEXT_EXECUTABLE,
	MAGIC_BCPL_SOURCE_TEXT,
	MAGIC_C_SOURCE_TEXT,
	MAGIC_CPP_SOURCE_TEXT,
	MAGIC_UNIFIED_DIFF_OUTPUT_TEXT,
	MAGIC_FORTRAN_PROGRAM,
	MAGIC_GNU_AWK_SCRIPT_TEXT_EXECUTABLE,
	MAGIC_GNU_INFO_TEXT,
	MAGIC_JAVA_SOURCE,
	MAGIC_LISP_SCHEME_PROGRAM_TEXT,
	MAGIC_LUA_SCRIPT_TEXT_EXECUTABLE,
	MAGIC_M4_MACRO_PROCESSOR_SCRIPT_TEXT,
	MAGIC_MAKEFILE_SCRIPT_TEXT,
	MAGIC_DOS_BATCH_FILE_TEXT,
	MAGIC_NEW_AWK_SCRIPT_TEXT_EXECUTABLE,
	MAGIC_PASCAL_SOURCE_TEXT,
	MAGIC_PERL_SCRIPT_TEXT,
	MAGIC_PHP_SCRIPT_TEXT,
	MAGIC_GNU_GETTEXT_MESSAGE_CATALOGUE_TEXT,
	MAGIC_PYTHON_SCRIPT_TEXT_EXECUTABLE,
	MAGIC_RUBY_SOURCE_TEXT,
	MAGIC_SHELLSCRIPT_TEXT_EXECUTABLE,
	MAGIC_TCL_TK_SCRIPT_TEXT_EXECUTABLE,
	MAGIC_LATEX_FILE,
	MAGIC_TEXINFO_SOURCE_TEXT,
	MAGIC_VCARD_VISITING_CARD,
	MAGIC_XMCD_DATABASE_FILE_FOR_KSCD,
	MAGIC_MPEG_SEQUENCE,
	MAGIC_APPLE_QUICKTIME_MOVIE,
	MAGIC_WEBM,
	MAGIC_FLC_ANIMATION,
	MAGIC_FLI_ANIMATION__320X200X8,
	MAGIC_MACROMEDIA_FLASH_VIDEO,
	MAGIC_JNG_VIDEO_DATA_,
	MAGIC_MNG_VIDEO_DATA_,
	MAGIC_MICROSOFT_ASF,
	MAGIC_SILICON_GRAPHICS_MOVIE_FILE,
	MAGIC_SYMBIAN_INSTALLATION_FILE__SYMBIAN_OS_9_X_,
	MAGIC_MAX	/* last element */
};

/*
 * End of autogenerated section.
 */

struct peak_magic	*peak_magic_init(void);
void			 peak_magic_exit(struct peak_magic *);
unsigned int		 peak_magic_get(struct peak_magic *,
			     const char *, const size_t);
unsigned int		 peak_magic_number(const char *);
const char		*peak_magic_name(const unsigned int);
const char		*peak_magic_desc(const unsigned int);

#endif /* PEAK_MAGIC_H */
