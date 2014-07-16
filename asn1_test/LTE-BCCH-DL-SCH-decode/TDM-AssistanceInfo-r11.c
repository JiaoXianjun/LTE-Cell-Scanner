/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "EUTRA-RRC-Definitions"
 * 	found in "36331-c10.asn"
 * 	`asn1c -S /usr/local/share/asn1c -fcompound-names -fskeletons-copy -gen-PER`
 */

#include "TDM-AssistanceInfo-r11.h"

static int
drx_CycleLength_r11_3_constraint(asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	/* Replace with underlying type checker */
	td->check_constraints = asn_DEF_NativeEnumerated.check_constraints;
	return td->check_constraints(td, sptr, ctfailcb, app_key);
}

/*
 * This type is implemented using NativeEnumerated,
 * so here we adjust the DEF accordingly.
 */
static void
drx_CycleLength_r11_3_inherit_TYPE_descriptor(asn_TYPE_descriptor_t *td) {
	td->free_struct    = asn_DEF_NativeEnumerated.free_struct;
	td->print_struct   = asn_DEF_NativeEnumerated.print_struct;
	td->check_constraints = asn_DEF_NativeEnumerated.check_constraints;
	td->ber_decoder    = asn_DEF_NativeEnumerated.ber_decoder;
	td->der_encoder    = asn_DEF_NativeEnumerated.der_encoder;
	td->xer_decoder    = asn_DEF_NativeEnumerated.xer_decoder;
	td->xer_encoder    = asn_DEF_NativeEnumerated.xer_encoder;
	td->uper_decoder   = asn_DEF_NativeEnumerated.uper_decoder;
	td->uper_encoder   = asn_DEF_NativeEnumerated.uper_encoder;
	if(!td->per_constraints)
		td->per_constraints = asn_DEF_NativeEnumerated.per_constraints;
	td->elements       = asn_DEF_NativeEnumerated.elements;
	td->elements_count = asn_DEF_NativeEnumerated.elements_count;
     /* td->specifics      = asn_DEF_NativeEnumerated.specifics;	// Defined explicitly */
}

static void
drx_CycleLength_r11_3_free(asn_TYPE_descriptor_t *td,
		void *struct_ptr, int contents_only) {
	drx_CycleLength_r11_3_inherit_TYPE_descriptor(td);
	td->free_struct(td, struct_ptr, contents_only);
}

static int
drx_CycleLength_r11_3_print(asn_TYPE_descriptor_t *td, const void *struct_ptr,
		int ilevel, asn_app_consume_bytes_f *cb, void *app_key) {
	drx_CycleLength_r11_3_inherit_TYPE_descriptor(td);
	return td->print_struct(td, struct_ptr, ilevel, cb, app_key);
}

static asn_dec_rval_t
drx_CycleLength_r11_3_decode_ber(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		void **structure, const void *bufptr, size_t size, int tag_mode) {
	drx_CycleLength_r11_3_inherit_TYPE_descriptor(td);
	return td->ber_decoder(opt_codec_ctx, td, structure, bufptr, size, tag_mode);
}

static asn_enc_rval_t
drx_CycleLength_r11_3_encode_der(asn_TYPE_descriptor_t *td,
		void *structure, int tag_mode, ber_tlv_tag_t tag,
		asn_app_consume_bytes_f *cb, void *app_key) {
	drx_CycleLength_r11_3_inherit_TYPE_descriptor(td);
	return td->der_encoder(td, structure, tag_mode, tag, cb, app_key);
}

static asn_dec_rval_t
drx_CycleLength_r11_3_decode_xer(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		void **structure, const char *opt_mname, const void *bufptr, size_t size) {
	drx_CycleLength_r11_3_inherit_TYPE_descriptor(td);
	return td->xer_decoder(opt_codec_ctx, td, structure, opt_mname, bufptr, size);
}

static asn_enc_rval_t
drx_CycleLength_r11_3_encode_xer(asn_TYPE_descriptor_t *td, void *structure,
		int ilevel, enum xer_encoder_flags_e flags,
		asn_app_consume_bytes_f *cb, void *app_key) {
	drx_CycleLength_r11_3_inherit_TYPE_descriptor(td);
	return td->xer_encoder(td, structure, ilevel, flags, cb, app_key);
}

static asn_dec_rval_t
drx_CycleLength_r11_3_decode_uper(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		asn_per_constraints_t *constraints, void **structure, asn_per_data_t *per_data) {
	drx_CycleLength_r11_3_inherit_TYPE_descriptor(td);
	return td->uper_decoder(opt_codec_ctx, td, constraints, structure, per_data);
}

static asn_enc_rval_t
drx_CycleLength_r11_3_encode_uper(asn_TYPE_descriptor_t *td,
		asn_per_constraints_t *constraints,
		void *structure, asn_per_outp_t *per_out) {
	drx_CycleLength_r11_3_inherit_TYPE_descriptor(td);
	return td->uper_encoder(td, constraints, structure, per_out);
}

static int
drx_ActiveTime_r11_13_constraint(asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	/* Replace with underlying type checker */
	td->check_constraints = asn_DEF_NativeEnumerated.check_constraints;
	return td->check_constraints(td, sptr, ctfailcb, app_key);
}

/*
 * This type is implemented using NativeEnumerated,
 * so here we adjust the DEF accordingly.
 */
static void
drx_ActiveTime_r11_13_inherit_TYPE_descriptor(asn_TYPE_descriptor_t *td) {
	td->free_struct    = asn_DEF_NativeEnumerated.free_struct;
	td->print_struct   = asn_DEF_NativeEnumerated.print_struct;
	td->check_constraints = asn_DEF_NativeEnumerated.check_constraints;
	td->ber_decoder    = asn_DEF_NativeEnumerated.ber_decoder;
	td->der_encoder    = asn_DEF_NativeEnumerated.der_encoder;
	td->xer_decoder    = asn_DEF_NativeEnumerated.xer_decoder;
	td->xer_encoder    = asn_DEF_NativeEnumerated.xer_encoder;
	td->uper_decoder   = asn_DEF_NativeEnumerated.uper_decoder;
	td->uper_encoder   = asn_DEF_NativeEnumerated.uper_encoder;
	if(!td->per_constraints)
		td->per_constraints = asn_DEF_NativeEnumerated.per_constraints;
	td->elements       = asn_DEF_NativeEnumerated.elements;
	td->elements_count = asn_DEF_NativeEnumerated.elements_count;
     /* td->specifics      = asn_DEF_NativeEnumerated.specifics;	// Defined explicitly */
}

static void
drx_ActiveTime_r11_13_free(asn_TYPE_descriptor_t *td,
		void *struct_ptr, int contents_only) {
	drx_ActiveTime_r11_13_inherit_TYPE_descriptor(td);
	td->free_struct(td, struct_ptr, contents_only);
}

static int
drx_ActiveTime_r11_13_print(asn_TYPE_descriptor_t *td, const void *struct_ptr,
		int ilevel, asn_app_consume_bytes_f *cb, void *app_key) {
	drx_ActiveTime_r11_13_inherit_TYPE_descriptor(td);
	return td->print_struct(td, struct_ptr, ilevel, cb, app_key);
}

static asn_dec_rval_t
drx_ActiveTime_r11_13_decode_ber(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		void **structure, const void *bufptr, size_t size, int tag_mode) {
	drx_ActiveTime_r11_13_inherit_TYPE_descriptor(td);
	return td->ber_decoder(opt_codec_ctx, td, structure, bufptr, size, tag_mode);
}

static asn_enc_rval_t
drx_ActiveTime_r11_13_encode_der(asn_TYPE_descriptor_t *td,
		void *structure, int tag_mode, ber_tlv_tag_t tag,
		asn_app_consume_bytes_f *cb, void *app_key) {
	drx_ActiveTime_r11_13_inherit_TYPE_descriptor(td);
	return td->der_encoder(td, structure, tag_mode, tag, cb, app_key);
}

static asn_dec_rval_t
drx_ActiveTime_r11_13_decode_xer(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		void **structure, const char *opt_mname, const void *bufptr, size_t size) {
	drx_ActiveTime_r11_13_inherit_TYPE_descriptor(td);
	return td->xer_decoder(opt_codec_ctx, td, structure, opt_mname, bufptr, size);
}

static asn_enc_rval_t
drx_ActiveTime_r11_13_encode_xer(asn_TYPE_descriptor_t *td, void *structure,
		int ilevel, enum xer_encoder_flags_e flags,
		asn_app_consume_bytes_f *cb, void *app_key) {
	drx_ActiveTime_r11_13_inherit_TYPE_descriptor(td);
	return td->xer_encoder(td, structure, ilevel, flags, cb, app_key);
}

static asn_dec_rval_t
drx_ActiveTime_r11_13_decode_uper(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		asn_per_constraints_t *constraints, void **structure, asn_per_data_t *per_data) {
	drx_ActiveTime_r11_13_inherit_TYPE_descriptor(td);
	return td->uper_decoder(opt_codec_ctx, td, constraints, structure, per_data);
}

static asn_enc_rval_t
drx_ActiveTime_r11_13_encode_uper(asn_TYPE_descriptor_t *td,
		asn_per_constraints_t *constraints,
		void *structure, asn_per_outp_t *per_out) {
	drx_ActiveTime_r11_13_inherit_TYPE_descriptor(td);
	return td->uper_encoder(td, constraints, structure, per_out);
}

static int
memb_drx_Offset_r11_constraint_2(asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	long value;
	
	if(!sptr) {
		_ASN_CTFAIL(app_key, td, sptr,
			"%s: value not given (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
	
	value = *(const long *)sptr;
	
	if((value >= 0 && value <= 255)) {
		/* Constraint check succeeded */
		return 0;
	} else {
		_ASN_CTFAIL(app_key, td, sptr,
			"%s: constraint failed (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
}

static asn_per_constraints_t asn_PER_type_drx_CycleLength_r11_constr_3 GCC_NOTUSED = {
	{ APC_CONSTRAINED,	 3,  3,  0,  7 }	/* (0..7) */,
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	0, 0	/* No PER value map */
};
static asn_per_constraints_t asn_PER_type_drx_ActiveTime_r11_constr_13 GCC_NOTUSED = {
	{ APC_CONSTRAINED,	 3,  3,  0,  7 }	/* (0..7) */,
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	0, 0	/* No PER value map */
};
static asn_per_constraints_t asn_PER_memb_drx_Offset_r11_constr_12 GCC_NOTUSED = {
	{ APC_CONSTRAINED,	 8,  8,  0,  255 }	/* (0..255) */,
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	0, 0	/* No PER value map */
};
static asn_per_constraints_t asn_PER_type_TDM_AssistanceInfo_r11_constr_1 GCC_NOTUSED = {
	{ APC_CONSTRAINED | APC_EXTENSIBLE,  1,  1,  0,  1 }	/* (0..1,...) */,
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	0, 0	/* No PER value map */
};
static asn_INTEGER_enum_map_t asn_MAP_drx_CycleLength_r11_value2enum_3[] = {
	{ 0,	4,	"sf40" },
	{ 1,	4,	"sf64" },
	{ 2,	4,	"sf80" },
	{ 3,	5,	"sf128" },
	{ 4,	5,	"sf160" },
	{ 5,	5,	"sf256" },
	{ 6,	6,	"spare2" },
	{ 7,	6,	"spare1" }
};
static unsigned int asn_MAP_drx_CycleLength_r11_enum2value_3[] = {
	3,	/* sf128(3) */
	4,	/* sf160(4) */
	5,	/* sf256(5) */
	0,	/* sf40(0) */
	1,	/* sf64(1) */
	2,	/* sf80(2) */
	7,	/* spare1(7) */
	6	/* spare2(6) */
};
static asn_INTEGER_specifics_t asn_SPC_drx_CycleLength_r11_specs_3 = {
	asn_MAP_drx_CycleLength_r11_value2enum_3,	/* "tag" => N; sorted by tag */
	asn_MAP_drx_CycleLength_r11_enum2value_3,	/* N => "tag"; sorted by N */
	8,	/* Number of elements in the maps */
	0,	/* Enumeration is not extensible */
	1,	/* Strict enumeration */
	0,	/* Native long size */
	0
};
static ber_tlv_tag_t asn_DEF_drx_CycleLength_r11_tags_3[] = {
	(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (10 << 2))
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_drx_CycleLength_r11_3 = {
	"drx-CycleLength-r11",
	"drx-CycleLength-r11",
	drx_CycleLength_r11_3_free,
	drx_CycleLength_r11_3_print,
	drx_CycleLength_r11_3_constraint,
	drx_CycleLength_r11_3_decode_ber,
	drx_CycleLength_r11_3_encode_der,
	drx_CycleLength_r11_3_decode_xer,
	drx_CycleLength_r11_3_encode_xer,
	drx_CycleLength_r11_3_decode_uper,
	drx_CycleLength_r11_3_encode_uper,
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_drx_CycleLength_r11_tags_3,
	sizeof(asn_DEF_drx_CycleLength_r11_tags_3)
		/sizeof(asn_DEF_drx_CycleLength_r11_tags_3[0]) - 1, /* 1 */
	asn_DEF_drx_CycleLength_r11_tags_3,	/* Same as above */
	sizeof(asn_DEF_drx_CycleLength_r11_tags_3)
		/sizeof(asn_DEF_drx_CycleLength_r11_tags_3[0]), /* 2 */
	&asn_PER_type_drx_CycleLength_r11_constr_3,
	0, 0,	/* Defined elsewhere */
	&asn_SPC_drx_CycleLength_r11_specs_3	/* Additional specs */
};

static asn_INTEGER_enum_map_t asn_MAP_drx_ActiveTime_r11_value2enum_13[] = {
	{ 0,	4,	"sf20" },
	{ 1,	4,	"sf30" },
	{ 2,	4,	"sf40" },
	{ 3,	4,	"sf60" },
	{ 4,	4,	"sf80" },
	{ 5,	5,	"sf100" },
	{ 6,	6,	"spare2" },
	{ 7,	6,	"spare1" }
};
static unsigned int asn_MAP_drx_ActiveTime_r11_enum2value_13[] = {
	5,	/* sf100(5) */
	0,	/* sf20(0) */
	1,	/* sf30(1) */
	2,	/* sf40(2) */
	3,	/* sf60(3) */
	4,	/* sf80(4) */
	7,	/* spare1(7) */
	6	/* spare2(6) */
};
static asn_INTEGER_specifics_t asn_SPC_drx_ActiveTime_r11_specs_13 = {
	asn_MAP_drx_ActiveTime_r11_value2enum_13,	/* "tag" => N; sorted by tag */
	asn_MAP_drx_ActiveTime_r11_enum2value_13,	/* N => "tag"; sorted by N */
	8,	/* Number of elements in the maps */
	0,	/* Enumeration is not extensible */
	1,	/* Strict enumeration */
	0,	/* Native long size */
	0
};
static ber_tlv_tag_t asn_DEF_drx_ActiveTime_r11_tags_13[] = {
	(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (10 << 2))
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_drx_ActiveTime_r11_13 = {
	"drx-ActiveTime-r11",
	"drx-ActiveTime-r11",
	drx_ActiveTime_r11_13_free,
	drx_ActiveTime_r11_13_print,
	drx_ActiveTime_r11_13_constraint,
	drx_ActiveTime_r11_13_decode_ber,
	drx_ActiveTime_r11_13_encode_der,
	drx_ActiveTime_r11_13_decode_xer,
	drx_ActiveTime_r11_13_encode_xer,
	drx_ActiveTime_r11_13_decode_uper,
	drx_ActiveTime_r11_13_encode_uper,
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_drx_ActiveTime_r11_tags_13,
	sizeof(asn_DEF_drx_ActiveTime_r11_tags_13)
		/sizeof(asn_DEF_drx_ActiveTime_r11_tags_13[0]) - 1, /* 1 */
	asn_DEF_drx_ActiveTime_r11_tags_13,	/* Same as above */
	sizeof(asn_DEF_drx_ActiveTime_r11_tags_13)
		/sizeof(asn_DEF_drx_ActiveTime_r11_tags_13[0]), /* 2 */
	&asn_PER_type_drx_ActiveTime_r11_constr_13,
	0, 0,	/* Defined elsewhere */
	&asn_SPC_drx_ActiveTime_r11_specs_13	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_drx_AssistanceInfo_r11_2[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct TDM_AssistanceInfo_r11__drx_AssistanceInfo_r11, drx_CycleLength_r11),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_drx_CycleLength_r11_3,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"drx-CycleLength-r11"
		},
	{ ATF_POINTER, 1, offsetof(struct TDM_AssistanceInfo_r11__drx_AssistanceInfo_r11, drx_Offset_r11),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_NativeInteger,
		memb_drx_Offset_r11_constraint_2,
		&asn_PER_memb_drx_Offset_r11_constr_12,
		0,
		"drx-Offset-r11"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct TDM_AssistanceInfo_r11__drx_AssistanceInfo_r11, drx_ActiveTime_r11),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_drx_ActiveTime_r11_13,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"drx-ActiveTime-r11"
		},
};
static int asn_MAP_drx_AssistanceInfo_r11_oms_2[] = { 1 };
static ber_tlv_tag_t asn_DEF_drx_AssistanceInfo_r11_tags_2[] = {
	(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_drx_AssistanceInfo_r11_tag2el_2[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* drx-CycleLength-r11 */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* drx-Offset-r11 */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 } /* drx-ActiveTime-r11 */
};
static asn_SEQUENCE_specifics_t asn_SPC_drx_AssistanceInfo_r11_specs_2 = {
	sizeof(struct TDM_AssistanceInfo_r11__drx_AssistanceInfo_r11),
	offsetof(struct TDM_AssistanceInfo_r11__drx_AssistanceInfo_r11, _asn_ctx),
	asn_MAP_drx_AssistanceInfo_r11_tag2el_2,
	3,	/* Count of tags in the map */
	asn_MAP_drx_AssistanceInfo_r11_oms_2,	/* Optional members */
	1, 0,	/* Root/Additions */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_drx_AssistanceInfo_r11_2 = {
	"drx-AssistanceInfo-r11",
	"drx-AssistanceInfo-r11",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	SEQUENCE_decode_uper,
	SEQUENCE_encode_uper,
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_drx_AssistanceInfo_r11_tags_2,
	sizeof(asn_DEF_drx_AssistanceInfo_r11_tags_2)
		/sizeof(asn_DEF_drx_AssistanceInfo_r11_tags_2[0]) - 1, /* 1 */
	asn_DEF_drx_AssistanceInfo_r11_tags_2,	/* Same as above */
	sizeof(asn_DEF_drx_AssistanceInfo_r11_tags_2)
		/sizeof(asn_DEF_drx_AssistanceInfo_r11_tags_2[0]), /* 2 */
	0,	/* No PER visible constraints */
	asn_MBR_drx_AssistanceInfo_r11_2,
	3,	/* Elements count */
	&asn_SPC_drx_AssistanceInfo_r11_specs_2	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_TDM_AssistanceInfo_r11_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct TDM_AssistanceInfo_r11, choice.drx_AssistanceInfo_r11),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		0,
		&asn_DEF_drx_AssistanceInfo_r11_2,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"drx-AssistanceInfo-r11"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct TDM_AssistanceInfo_r11, choice.idc_SubframePatternList_r11),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_IDC_SubframePatternList_r11,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"idc-SubframePatternList-r11"
		},
};
static asn_TYPE_tag2member_t asn_MAP_TDM_AssistanceInfo_r11_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* drx-AssistanceInfo-r11 */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 } /* idc-SubframePatternList-r11 */
};
static asn_CHOICE_specifics_t asn_SPC_TDM_AssistanceInfo_r11_specs_1 = {
	sizeof(struct TDM_AssistanceInfo_r11),
	offsetof(struct TDM_AssistanceInfo_r11, _asn_ctx),
	offsetof(struct TDM_AssistanceInfo_r11, present),
	sizeof(((struct TDM_AssistanceInfo_r11 *)0)->present),
	asn_MAP_TDM_AssistanceInfo_r11_tag2el_1,
	2,	/* Count of tags in the map */
	0,
	2	/* Extensions start */
};
asn_TYPE_descriptor_t asn_DEF_TDM_AssistanceInfo_r11 = {
	"TDM-AssistanceInfo-r11",
	"TDM-AssistanceInfo-r11",
	CHOICE_free,
	CHOICE_print,
	CHOICE_constraint,
	CHOICE_decode_ber,
	CHOICE_encode_der,
	CHOICE_decode_xer,
	CHOICE_encode_xer,
	CHOICE_decode_uper,
	CHOICE_encode_uper,
	CHOICE_outmost_tag,
	0,	/* No effective tags (pointer) */
	0,	/* No effective tags (count) */
	0,	/* No tags (pointer) */
	0,	/* No tags (count) */
	&asn_PER_type_TDM_AssistanceInfo_r11_constr_1,
	asn_MBR_TDM_AssistanceInfo_r11_1,
	2,	/* Elements count */
	&asn_SPC_TDM_AssistanceInfo_r11_specs_1	/* Additional specs */
};
