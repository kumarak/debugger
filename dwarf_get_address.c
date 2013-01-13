#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void die(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args)
	
	exit(EXIT_FAILURE);
}
/* This will list the function in a DIE*/
void list_func_in_die(Dwarf_Debug dbg, Dwarf_Die the_die)
{
	char* die_name = NULL;
	const char* tag_name = NULL;
	Dwarf_Error err;
	Dwarf_Half tag;
	Dwarf_Attribute* attrs;
	Dwarf_Addr lowpc, highpc;
	Dwarf_Signed attrcount, i;

	int rc = dwarf_diename(the_die, &die_name, &err);

	if(rc == DW_DLV_ERROR)
		die("Error in dwarf_diename\n");
	else if(rc == DW_DLV_NO_ENTRY)
		return;

	if(dwarf_tag(the_die, &tag, &err) != DW_DLV_OK)
		die("Error in dwarf_tag\n");
	
	if(tag != DW_TAG_subprogram)
		return;
	
	if(dwarf_get_TAG_name(tag, &tag_name) != DW_DLV_OK)
		die("Error in dearf_get_TAG_name\n");

	printf("DW_TAG_subprogram: '%s'\n", die_name);
	
	if(dwarf_attrlist(the_die, &attrs, &attrcount, &err) != DW_DLV_OK)
		die("Error in dwarf_attrlist\n");	 
}

void list_funcs_in_file(Dwarf_Debug dbg)
{
	Dwarf_Unsigned cu_header_length, abbrev_offset, next_cu_header;
	Dwarf_Half version_stamp, address_size;
	Dwarf_Error err;
	Dwarf_Die no_die = 0, cu_die, child_die;

	/* Find compilation unit headers*/
	if(dwarf_next_cu_header(
		dbg,
		&cu_header_length,
		&version_stamp,
		&abbrev_offset,
		&address_size,
		&next_cu_header,
		&err) == DW_DLV_ERROR){
			die("Error reading DWARF cu headers\n");
	}

	/* Expect the CU to have a single sibling - a DIE*/
	if(dwarf_siblingof(dbg, no_die, &cu_die, &err) == DW_DLV_ERROR){
		die("Error getting sibling of CU\n");
	}
	
	/* Expect the CU DIE to have children*/
	if(dwarf_child(cu_die, &child_die, &err) == DW_DLV_ERROR){
		die("Error getting child of CU DIE\n");
	}

	/* Go over all children DIEs*/
	while(1){
		int rc;
		list_func_in_die(dbg, child_die);
		rc = dwarf_siblingof(dbg, child_die, &child_die, &err);
		
		if(rc == DW_DLV_ERROR)
			die("Error getting sibling of DIE\n");
		else if(rc == DW_DLV_NO_ENTRY)
			break;
	}
}


int main(int argc, char** argv)
{
	Dwarf_Debug dbg = 0;
	Dwarf_Error err = 0;
	const char* progname;
	int fd = -1;
	
	if(argc < 2) {
		fprintf(stderr, "Expected a program name as argument\n");
		return 1;
	}

	progname = argv[1];
	if(fd = open(progname, O_RDONLY) < 0){
		perror("opne");
		return 1;
	}

	if(dwarf_init(fd, DW_DLC_READ, 0, 0, &dbg, &err) != DW_DLV_OK){
		fprintf(stderr, "Failed DWARF Initialization\n");
		return 1;
	}
	
	list_funcs_in_file(dbg);

	if(dwarf_finish(dbg, &err) != DW_DLV_OK){
		fprintf(stderr,"Failed DWARF Finalization\n");
		return 1;
	}
}
