// cao p1 at-1.cpp : Defines the entry point for the console application.
//1.add,2.sub,3.and,4.or,5.xor,6.movc,7.load,8.store,9.bz,10.bnz,11.jump,12.bal,13.halt,14.nop
#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
FILE *fp;
struct list
{
	char instr[100];
	int pos;
	struct list *next, *prev;
};
struct reg_to_value
{
	int free;//0=free 1= taken
	int valid;//0=invalid 1=valid
	char reg[5];
	int rat_index;
	int value_in;
};
struct decoded_instructions
{
	int pc;
	int ins_type;
	int drnum;
	int dreg_type;
	int dest_valid;
	int dest_val;
	int sreg1_valid;
	int srnum1;
	int sreg1_type;
	int src1_val;
	int sreg2_valid;
	int srnum2;
	int sreg2_type;
	int src2_val;
	struct decoded_instructions* next;
	struct decoded_instructions* prev;
};
struct rat
{
	int free_taken;//0=free 1=taken
	char src[3];
	int src_val;
	int srnum;
	char dest[5];
	int drnum;
	int dest_val;
};
int r[32] = { '\0' };
int p[500] = { '\0' };
int X = '\0';
int valid[32] = { 1 };
int address[1000] = { '\0' };
int valid_address[1000] = { 1 };
int num = 20000;
int lsq_delay = 3, mul_delay = 4;
struct decoded_instructions *iq_head = NULL, *iq_tail = NULL, *iq_curr, *ecurr_lsq, *ecurr_mul, *ecurr_int, *mcurr, *wcurr;
struct decoded_instructions *rob_head = NULL, *rob_tail = NULL, *rob_curr = NULL;
static struct list *head = NULL, *curr = NULL, *temp, *fcurr, *dcurr, *ecurr;
struct rat rat_table[1000];
struct reg_to_value p_reg[1000];
int address_transfer = 0;
int max_p_reg=0, max_rat=0;
void load()
{
	char string[100];
	while (fgets(string, 100, (FILE*)fp))
	{
		if (!head)
		{
			temp = malloc(sizeof(struct list));
			temp->next = NULL;
			temp->prev = NULL;
			strcpy(temp->instr, string);
			num++;
			temp->pos = num;
			curr = temp;
			head = curr;
		}
		else
		{
			temp = malloc(sizeof(struct list));
			temp->next = NULL;
			temp->prev = curr;
			num++;
			temp->pos = num;
			strcpy(temp->instr, string);
			curr->next = temp;
			curr = curr->next;
		}
	}
}
void vdisplay()
{
	temp = head;
	while (temp != NULL)
	{
		printf("instr %d %s\n", temp->pos, temp->instr);
		temp = temp->next;
	}

}
int fetch(struct list  *fetchptr)
{
	if (fetchptr)
	{
		if (!dcurr)
		{
			dcurr = fcurr;
			//fcurr = NULL;
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

int check_type_of_ins(struct list *raw)
{
	//1.add,2.sub,3.and,4.or,5.xor,6.movc,7.load,8.store,9.bz,10.bnz,11.jump,12.bal,13.halt,14.nop15.mul
	int type = 0;
	const char space[5] = " ,#\t";
	char* ins;// = (char*)malloc(sizeof(char) * 5);

	char *dcurr_str = (char*)malloc(sizeof(char) * 10);
	strcpy(dcurr_str, raw->instr);
	ins = strtok(dcurr_str, space);
	if (!strncmp(ins, "MOVC", 4) || !strncmp(ins, "MOV", 3)) { return 6; }
	else if (!strncmp(ins, "ADD", 3)) { return 1; }
	else if (!strncmp(ins, "SUB", 3)) { return 2; }
	else if (!strncmp(ins, "AND", 3)) { return 3; }
	else if (!strncmp(ins, "OR", 2)) { return 4; }
	else if (!strncmp(ins, "XOR", 3)) { return 5; }
	else if (!strncmp(ins, "MUL", 3)) { return 15; }
	else if (!strncmp(ins, "LOAD", 4)) { return 7; }
	else if (!strncmp(ins, "STORE", 5)) { return 8; }
	else if (!strncmp(ins, "BZ", 2)) { return 9; }
	else if (!strncmp(ins, "BNZ", 3)) { return 10; }
	else if (!strncmp(ins, "BAl", 3)) { return 12; }
	else if (!strncmp(ins, "JUMP", 4)) { return 11; }
	else if (!strncmp(ins, "HALT", 4)) { return 13; }
	else if (!strncmp(ins, "NOP", 3)) { return 14; }
	else { return 14; }
	//free(dcurr_str);
}
int rat_link()
{
	int i = 0;
	for (i = 1; i <=
		500; i++)
	{
		if (rat_table[i].free_taken == 0) { break; }
	}
	return i;
}
int find_free_preg()
{
	int i;
	for (i = 1; i <= 500; i++)
	{
		if (p_reg[i].free == 0)
		{
			break;
		}
	}
	return i;
}

int rat_logic(char* dreg,char* reg1,int rnum)
{
	int rat_index, preg_index;
	rat_index = rat_link();
	if(max_rat<rat_index){ max_rat = rat_index; }
	preg_index = find_free_preg();
	if (max_p_reg < preg_index) { max_p_reg = preg_index; }
	char preg[3];
	sprintf(preg, "p%d", preg_index);
	p_reg[preg_index].free = 1;
	p_reg[preg_index].rat_index = rat_index;
	strcpy(rat_table[rat_index].src, dreg);
	rat_table[rat_index].src_val = '\0';
	rat_table[rat_index].srnum = rnum;
	rat_table[rat_index].drnum = preg_index;
	rat_table[rat_index].free_taken = 1;
	strcpy(p_reg[preg_index].reg, preg);
	strcpy(rat_table[rat_index].dest, preg);
	return preg_index;
}

void rob_generate(struct decoded_instructions *temp_decoded)
{
	struct decoded_instructions *temp_var;
	temp_var = (struct decoded_instructions*)malloc(sizeof(struct decoded_instructions));
	temp_var->dest_val = temp_decoded->dest_val;
	temp_var->dreg_type = temp_decoded->dreg_type;
	temp_var->drnum = temp_decoded->drnum;
	temp_var->ins_type = temp_decoded->ins_type;
	temp_var->pc = temp_decoded->pc;
	temp_var->src1_val = temp_decoded->src1_val;
	temp_var->src2_val = temp_decoded->src2_val;
	temp_var->sreg1_type = temp_decoded->sreg1_type;
	temp_var->sreg1_valid = temp_decoded->sreg1_valid;
	temp_var->sreg2_type = temp_decoded->sreg2_type;
	temp_var->sreg2_valid = temp_decoded->sreg2_valid;
	temp_var->srnum1 = temp_decoded->srnum1;
	temp_var->srnum2 = temp_decoded->srnum2;
	if (!rob_head)
	{
		rob_head = temp_var;
		rob_curr = rob_head;
		rob_head->prev = NULL;
		rob_head->next = NULL;
	}
	else
	{
		rob_curr->next = temp_var;
		temp_var->prev = rob_curr;
		rob_curr = rob_curr->next;
	}
}
int find_rtop_link(int rnum,char* reg)
{
	int i;
	for (i = max_rat; i >=0; i--)
	{
		if (!strncmp(rat_table[i].src, reg, 3))
		{
			break;
		}
	}
	return i;
}
void decode()
{
	if (dcurr)
	{
		//1.add,2.sub,3.and,4.or,5.xor,6.movc,7.load,8.store,9.bz,10.bnz,11.jump,12.bal,13.halt,14.nop
		int rnumd = '\0', rnums1 = '\0', rnums2 = '\0';
		char *ins, *dreg, *reg1, *reg2;
		const char space[5] = " ,#\t\n";
		char *dcurr_str = (char*)malloc(sizeof(char) * 10);
		strcpy(dcurr_str, dcurr->instr);
		ins = strtok(dcurr_str, space);
		if (!strncmp(ins, "MOVC", 4))
		{
			dreg = strtok(NULL, space);
			reg1 = strtok(NULL, space);
			if (!dreg[2])
			{
				rnumd = (int)(dreg[1]) - 48;
			}
			else
			{
				rnumd = (int)(dreg[2] - '0');
				rnumd = rnumd + (10 * (int)(dreg[1] - '0'));
			}
			int preg_index = rat_logic(dreg, reg1, rnumd);
			int index = dcurr->pos - 20000;
			struct decoded_instructions* temp_ins = (struct decoded_instructions*)malloc(sizeof(struct decoded_instructions));
			temp_ins->ins_type = check_type_of_ins(dcurr);
			temp_ins->drnum = preg_index;
			temp_ins->dreg_type = 1;
			temp_ins->dest_valid = 0;
			if (!strncmp(reg1, "X", 1))
			{
				temp_ins->src1_val = X;
			}
			else
			{
				temp_ins->src1_val = atoi(reg1);
			}
			temp_ins->sreg1_valid = 1;
			temp_ins->sreg1_type = 0;
			temp_ins->pc = dcurr->pos;
			temp_ins->next = NULL;
			temp_ins->prev = NULL;
			if (!iq_head)
			{
				iq_head = temp_ins;
				iq_tail = temp_ins;
				iq_curr = iq_head;
			}
			else
			{
				iq_curr->next = temp_ins;
				temp_ins->prev = iq_curr;
				iq_curr = iq_curr->next;
				iq_tail = iq_curr;
			}
			rob_generate(iq_curr);			
		}
		else if (!strncmp(ins, "MOV", 3))
		{
			dreg = strtok(NULL, space);
			reg1 = strtok(NULL, space);
			if (!dreg[2])
			{
				rnumd = (int)(dreg[1]) - 48;
			}
			else
			{
				rnumd = (int)(dreg[2] - '0');
				rnumd = rnumd + (10 * (int)(dreg[1] - '0'));
			}
			int preg_index = rat_logic(dreg, reg1, rnumd);
			int index = dcurr->pos - 20000;
			struct decoded_instructions* temp_ins = (struct decoded_instructions*)malloc(sizeof(struct decoded_instructions));
			temp_ins->ins_type = check_type_of_ins(dcurr);
			temp_ins->drnum = preg_index;

			temp_ins->dreg_type = 1;
			if (!strncmp(reg1, "X", 1))
			{
				temp_ins->src1_val = X;
			}
			else
			{
				temp_ins->src1_val = atoi(reg1);
			}
			temp_ins->pc = dcurr->pos;
			temp_ins->next = NULL;
			temp_ins->prev = NULL;
			if (!iq_head)
			{
				iq_head = temp_ins;
				iq_tail = temp_ins;
				iq_curr = iq_head;
			}
			else
			{
				iq_curr->next = temp_ins;
				temp_ins->prev = iq_curr;
				iq_curr = iq_curr->next;
				iq_tail = iq_curr;
			}
			rob_generate(iq_curr);
		}
		else if (!strncmp(ins, "ADD", 3) || !strncmp(ins, "SUB", 3) || !strncmp(ins, "AND", 3) || !strncmp(ins, "OR", 2) || !strncmp(ins, "XOR", 3) || !strncmp(ins, "MUL", 3))
		{
			dreg = strtok(NULL, space);
			reg1 = strtok(NULL, space);
			reg2 = strtok(NULL, space);
			if (!dreg[2])
			{
				rnumd = (int)(dreg[1]) - 48;
			}
			else
			{
				rnumd = (int)(dreg[2] - '0');
				rnumd = rnumd + (10 * (int)(dreg[1] - '0'));
			}
			int preg_index = rat_logic(dreg, reg1, rnumd);
			int index = dcurr->pos - 20000;
			struct decoded_instructions* temp_ins = (struct decoded_instructions*)malloc(sizeof(struct decoded_instructions));
			temp_ins->ins_type = check_type_of_ins(dcurr);
			temp_ins->pc = dcurr->pos;			
			temp_ins->drnum = preg_index;
			p_reg[preg_index].valid = 0;
			temp_ins->dreg_type = 1;
			temp_ins->dest_valid = 0;
			if (!reg1[2])
			{
				rnums1 = (int)(reg1[1] - '0');
			}
			else
			{
				rnums1 = (int)(reg1[2] - '0');
				rnums1 = rnums1 + (10 * (int)(reg1[1] - '0'));
			}
			int rats1 = find_rtop_link(rnums1, reg1);
			int prnum = rat_table[rats1].drnum;
			temp_ins->srnum1 = prnum;
			temp_ins->sreg2_type = 1;
			if (p_reg[prnum].valid == 1)
			{
				temp_ins->sreg1_valid = 1;
				temp_ins->src1_val = p_reg[prnum].value_in;
				
			}
			else
			{
				temp_ins->sreg1_valid = 0;
			}
			if (!strncmp(reg2, "R", 1))
			{
				if (!reg2[2])
				{
					rnums2 = (int)(reg2[1] - '0');
				}
				else
				{
					rnums2 = (int)(reg2[2] - '0');
					rnums2 = rnums2 + (10 * (int)(reg2[1] - '0'));
				}
				rats1 = find_rtop_link(rnums2, reg2);
				prnum = rat_table[rats1].drnum;
				temp_ins->srnum2 = prnum;
				temp_ins->sreg2_type = 1;
				if (p_reg[prnum].valid == 1)
				{
					temp_ins->sreg2_valid = 1;
					temp_ins->src2_val = p_reg[prnum].value_in;					
					
				}
				
				else
				{
					temp_ins->sreg2_valid = 0;
				}
			}
			else
			{
				temp_ins->src2_val = atoi(reg2);
				temp_ins->sreg2_type = 0;
				temp_ins->sreg2_valid = 1;
			}
			temp_ins->next = NULL;
			temp_ins->prev = NULL;
			if (!iq_head)
			{
				iq_head = temp_ins;
				iq_tail = temp_ins;
				iq_curr = iq_head;
			}
			else
			{
				iq_curr = iq_head;
				while (iq_curr->next!=NULL)
				{
					iq_curr = iq_curr->next;
				}
				iq_curr->next = temp_ins;
				temp_ins->prev = iq_curr;
				iq_curr = iq_curr->next;
				iq_tail = iq_curr;
			}
			rob_generate(iq_curr);			
		}
		dcurr = NULL;
	}
}

int is_int(struct decoded_instructions *decoded)
{
	//1.add, 2.sub, 3.and, 4. or , 5.xor, 6.movc, 7.load, 8.store, 9.bz, 10.bnz, 11.jump, 12.bal, 13.halt, 14.nop
	if (decoded->ins_type >= 1 && decoded->ins_type <= 6)
		return 1;
	else
		return 0;
}
struct decoded_instructions *search_rob(int pc)
{
	struct decoded_instructions *temp=rob_curr;
	if (temp->pc == pc)
	{
		return temp;
	}
	else
	{
		temp = rob_head;
		while (temp != NULL)
		{
			if (temp->pc == pc)
			{
				break;
			}
			else
			{
				temp = temp->next;
			}
		}
	}
	
	return temp;
}
void delete_iq_entry(struct decoded_instructions *curr_iq)
{
	struct decoded_instructions *temp_iq,*temp_iq2;
	if (curr_iq != NULL)
	{
		if (curr_iq == iq_head)
		{			
			if(curr_iq->next!=NULL)
			{
				temp_iq = iq_head->next;
				temp_iq->prev = NULL;
				iq_head = temp_iq;
				free(curr_iq);
				curr_iq = NULL;
			}
			else
			{				
				free(curr_iq);
				curr_iq = NULL;
				iq_head = NULL;				
			}
			
		}
		else if (curr_iq == iq_tail)
		{
			if (iq_tail->prev !=NULL)
			{
				temp_iq = iq_tail->prev;
				temp_iq->next = NULL;

				free(curr_iq);
				curr_iq = NULL;
				iq_tail = temp_iq;
			}	
			else
			{
				free(curr_iq);
				curr_iq = NULL;
				iq_head = NULL;
				iq_tail = NULL;
			}
			
		}
		else
		{
			temp_iq = curr_iq->prev;
			temp_iq2 = curr_iq->next;
			if (temp_iq != NULL)
			{
				temp_iq->next = temp_iq2;
			}
			if (temp_iq2 != NULL)
			{
				temp_iq2->prev = temp_iq;
			}
			free(curr_iq);
			curr_iq = NULL;
		}
	}
	return;
}
void fu_lsq()
{
	if (!ecurr_lsq)
	{
		//1.add, 2.sub, 3.and, 4. or , 5.xor, 6.movc, 7.load, 8.store, 9.bz, 10.bnz, 11.jump, 12.bal, 13.halt, 14.nop
		struct decoded_instructions *fu_curr;
		fu_curr = iq_head;
		if (iq_head)
		{
			while (fu_curr)
			{
				if (fu_curr->ins_type == 7 || fu_curr->ins_type == 8)
				{
					switch (fu_curr->ins_type)
					{
					case 7:
						if (fu_curr->sreg1_valid)
						{
							ecurr_lsq = fu_curr;
						}
						break;
					case 8:
						if (fu_curr->sreg1_valid)
						{
							ecurr_lsq = fu_curr;
						}
						break;
					default:
						break;
					}
				}
				else
				{
					fu_curr = fu_curr->next;
				}
			}
			--lsq_delay;
			if (lsq_delay == 0)
			{
				if (!mcurr)
				{
					mcurr = ecurr_lsq;
				}
				ecurr_lsq = NULL;
				lsq_delay = 3;
			}
		}
	}
}
void fu_int()
{
	//1.add, 2.sub, 3.and, 4. or , 5.xor, 6.movc, 7.load, 8.store, 9.bz, 10.bnz, 11.jump, 12.bal, 13.halt, 14.nop
	if (!ecurr_int)
	{
		if (iq_head)
		{
			struct decoded_instructions *fu_curr,*rob_curr;
			fu_curr = iq_head;
			while (fu_curr)
			{
				if (is_int(fu_curr) == 1)
				{
					switch (fu_curr->ins_type)
					{
					case 1:
						if (fu_curr->sreg1_valid&& fu_curr->sreg2_valid)
						{
							ecurr_int = fu_curr;
							rob_curr = search_rob(fu_curr->pc);
							rob_curr->dest_val = fu_curr->src1_val + fu_curr->src2_val;
							rob_curr->dest_valid = 1;
							fu_curr->dest_val = fu_curr->src1_val + fu_curr->src2_val;
							fu_curr->dest_valid = 1;
							p_reg[fu_curr->drnum].value_in = fu_curr->dest_val;
							p_reg[fu_curr->drnum].valid = 1;
							p_reg[fu_curr->drnum].free = 1;
							p[fu_curr->drnum] = fu_curr->dest_val;
							rat_table[p_reg[fu_curr->drnum].rat_index].dest_val = fu_curr->dest_val;
							if (!mcurr)
							{
								mcurr = rob_curr;
								//delete the entry from iq							
								delete_iq_entry(ecurr_int);
								ecurr_int = NULL;
								return;
							}
							return;
						}
						else
							fu_curr = fu_curr->next;
						break;
					case 2:						
							if (fu_curr->sreg1_valid&& fu_curr->sreg2_valid)
							{
								ecurr_int = fu_curr;
								rob_curr = search_rob(fu_curr->pc);
								rob_curr->dest_val = fu_curr->src1_val - fu_curr->src2_val;
								rob_curr->dest_valid = 1;
								fu_curr->dest_val = fu_curr->src1_val - fu_curr->src2_val;
								fu_curr->dest_valid = 1;
								p_reg[fu_curr->drnum].value_in = fu_curr->dest_val;
								p_reg[fu_curr->drnum].valid = 1;
								p_reg[fu_curr->drnum].free = 1;
								p[fu_curr->drnum] = fu_curr->dest_val;
								rat_table[p_reg[fu_curr->drnum].rat_index].dest_val = fu_curr->dest_val;
									if (!mcurr)
									{
										mcurr = rob_curr;
										//delete the entry from iq							
										delete_iq_entry(ecurr_int);
										ecurr_int = NULL;
										return;
									}
									return;
							}

							else
								fu_curr = fu_curr->next;
							
						break;
					case 3:
							if (fu_curr->sreg1_valid&& fu_curr->sreg2_valid)
							{
								ecurr_int = fu_curr;
								rob_curr = search_rob(fu_curr->pc);
								rob_curr->dest_val = fu_curr->src1_val & fu_curr->src2_val;
								rob_curr->dest_valid = 1;
								fu_curr->dest_val = fu_curr->src1_val & fu_curr->src2_val;
								fu_curr->dest_valid = 1;
								p_reg[fu_curr->drnum].value_in = fu_curr->dest_val;
								p_reg[fu_curr->drnum].valid = 1;
								p_reg[fu_curr->drnum].free = 1;
								p[fu_curr->drnum] = fu_curr->dest_val;
								rat_table[p_reg[fu_curr->drnum].rat_index].dest_val = fu_curr->dest_val;
									if (!mcurr)
									{
										mcurr = rob_curr;
										//delete the entry from iq							
										delete_iq_entry(ecurr_int);
										ecurr_int = NULL;
										return;
									}
								return;
							}

							else
								fu_curr = fu_curr->next;
						
						break;
					case 4:
						
							if (fu_curr->sreg1_valid&& fu_curr->sreg2_valid)
							{
								ecurr_int = fu_curr;
								rob_curr = search_rob(fu_curr->pc);
								rob_curr->dest_val = fu_curr->src1_val | fu_curr->src2_val;
								rob_curr->dest_valid = 1;
								fu_curr->dest_val = fu_curr->src1_val | fu_curr->src2_val;
								fu_curr->dest_valid = 1;
								p_reg[fu_curr->drnum].value_in = fu_curr->dest_val;
								p_reg[fu_curr->drnum].valid = 1;
								p_reg[fu_curr->drnum].free = 1;
								p[fu_curr->drnum] = fu_curr->dest_val;
								rat_table[p_reg[fu_curr->drnum].rat_index].dest_val = fu_curr->dest_val;
									if (!mcurr)
									{
										mcurr = rob_curr;
										//delete the entry from iq							
										delete_iq_entry(ecurr_int);
										ecurr_int = NULL;
										return;
									}
								return;
							}

							else
								fu_curr = fu_curr->next;
						
						break;
					case 5:
						
							if (fu_curr->sreg1_valid&& fu_curr->sreg2_valid)
							{
								ecurr_int = fu_curr;
								rob_curr = search_rob(fu_curr->pc);
								rob_curr->dest_val = fu_curr->src1_val ^ fu_curr->src2_val;
								rob_curr->dest_valid = 1;
								fu_curr->dest_val = fu_curr->src1_val ^ fu_curr->src2_val;
								fu_curr->dest_valid = 1;
								p_reg[fu_curr->drnum].value_in = fu_curr->dest_val;
								p_reg[fu_curr->drnum].valid = 1;
								p_reg[fu_curr->drnum].free = 1;
								p[fu_curr->drnum] = fu_curr->dest_val;
								rat_table[p_reg[fu_curr->drnum].rat_index].dest_val = fu_curr->dest_val;
									if (!mcurr)
									{
										mcurr = rob_curr;
										//delete the entry from iq							
										delete_iq_entry(ecurr_int);
										ecurr_int = NULL;
										return;
									}
								return;
							}

							else
								fu_curr = fu_curr->next;
						
						break;
					case 6:
						ecurr_int = fu_curr;
						rob_curr=search_rob(fu_curr->pc);
						rob_curr->dest_val = fu_curr->src1_val;
						rob_curr->dest_valid = 1;						
						fu_curr->dest_val = fu_curr->src1_val;
						fu_curr->dest_valid = 1;
						p_reg[fu_curr->drnum].value_in = fu_curr->src1_val;
						p_reg[fu_curr->drnum].valid = 1;
						p_reg[fu_curr->drnum].free = 1;
						p[fu_curr->drnum] = fu_curr->src1_val;
						rat_table[p_reg[fu_curr->drnum].rat_index].dest_val = fu_curr->src1_val;
						rat_table[p_reg[fu_curr->drnum].rat_index].src_val = fu_curr->src1_val;
						if (!mcurr)
						{
							mcurr = rob_curr;
							//delete the entry from iq							
							delete_iq_entry(ecurr_int);
							ecurr_int = NULL;														
							return;
						}
						return;
						break;
					default:
						break;
					}
				}
				else
				{
					fu_curr = fu_curr->next;
				}
			}
		}
	}
	else
	{
		if (!mcurr)
		{
			mcurr = rob_curr;
			//delete the entry from iq							
			delete_iq_entry(ecurr_int);
			ecurr_int = NULL;
			return;
		}		
	}
}
void fu_mul()
{
	if (!ecurr_mul)
	{
		//1.add, 2.sub, 3.and, 4. or , 5.xor, 6.movc, 7.load, 8.store, 9.bz, 10.bnz, 11.jump, 12.bal, 13.halt, 14.nop, 15. mul
		struct decoded_instructions *fu_curr;
		fu_curr = iq_head;
		if (iq_head)
		{
			while (fu_curr)
			{
				if (fu_curr->ins_type == 15)
				{
					if (fu_curr->sreg1_valid&& fu_curr->sreg2_valid)
					{
						ecurr_mul = fu_curr;
						
						--mul_delay;
						return;
					}
				}
				else
				{
					fu_curr = fu_curr->next;
				}
			}						
		}		
	}
	else
	{
		--mul_delay;
		if (mul_delay == 0)
		{
			if (!mcurr)
			{
				
				//delete the entry from iq
				rob_curr = search_rob(ecurr_mul->pc);
				rob_curr->dest_val = ecurr_mul->src1_val * ecurr_mul->src2_val;
				rob_curr->dest_valid = 1;
				ecurr_mul->dest_val = ecurr_mul->src1_val * ecurr_mul->src2_val;
				ecurr_mul->dest_valid = 1;
				p_reg[ecurr_mul->drnum].value_in = ecurr_mul->dest_val;
				p_reg[ecurr_mul->drnum].valid = 1;
				p_reg[ecurr_mul->drnum].free = 1;
				p[ecurr_mul->drnum] = ecurr_mul->dest_val;
				rat_table[p_reg[ecurr_mul->drnum].rat_index].dest_val = ecurr_mul->dest_val;
				mcurr = rob_curr;
				delete_iq_entry(ecurr_mul);
				ecurr_mul = NULL;
				return;
			}
			mul_delay = 4;
		}		
	}
}
void update_iq()
{
	struct decoded_instructions *temp_update = iq_head;
	while (temp_update!=NULL)
	{
		if (temp_update->sreg1_valid == 0)
		{
			if (temp_update->sreg1_type == 1)
			{
				if (p_reg[temp_update->srnum1].valid == 1)
				{
					temp_update->sreg1_valid = 1;
					temp_update->src1_val = p_reg[temp_update->srnum1].value_in;
				}
			}
		}
		if (temp_update->sreg2_valid == 0)
		{
			if (temp_update->sreg2_type == 1)
			{
				if (p_reg[temp_update->srnum2].valid == 1)
				{
					temp_update->sreg2_valid = 1;
					temp_update->src2_val = p_reg[temp_update->srnum2].value_in;
				}
			}
		}
		temp_update = temp_update->next;
	}
}
void exec()
{
	update_iq();
	fu_int();
	fu_lsq();
	fu_mul();
}
void mem()
{
	if (mcurr)
	{
		wcurr = mcurr;
		mcurr = NULL;
	}
}
void wb()
{
	if (wcurr)
	{
		int i, rnum;
		for (i = 0; i < max_p_reg; i++)
		{
			if (!strncmp(p_reg[wcurr->drnum].reg, rat_table[i].dest, 5))
			{
				break;
			}
		}
		rat_table[i].src_val = wcurr->dest_val;
		rat_table[i].src;
		char *dreg = rat_table[i].src;
		if (!dreg[2])
		{
			rnum = (int)(dreg[1]) - 48;
		}
		else
		{
			rnum = (int)(dreg[2] - '0');
			rnum = rnum + (10 * (int)(dreg[1] - '0'));
		}
		r[rnum] = wcurr->dest_val;
		wcurr = NULL;
	}
}


int main()
{
	char *input, *instr1, *instr2; int i = 1, j;
	while (i != 0)
	{
		printf("\nPlease give the input from the following commands:\n\tload\n\tinitialize\n\tsimulate\n\tdisplay\n\tquit\n");
		input = (char*)malloc(sizeof(char) * 40);
		fgets(input, 40, stdin);
		if (!strncmp(input, "quit", 4))
		{
			i = 0;

		}
		else if (!strncmp(input, "load", 4))
		{
			printf("\n In load\n");
			strcat(input, " ");
			const char space[2] = " ";
			instr1 = strtok(input, space);
			instr2 = strtok(NULL, space);
			//free(instr2);
			if (instr2 != NULL)
			{

				instr2[strlen(instr2) - 1] = '\0';

				printf("\n|%s|\n", instr2);
				fp = fopen(instr2, "r");
				load();
				printf("load done\n");
				vdisplay();
				printf("\nAll Instructions Loaded done\n");
				fclose(fp);
			}
			else
			{
				printf("\nPlease enter the command in the format load <filename>");
			}
			//instr2[strlen(instr2)] = '\n';
			//free(instr1);
		}
		else if (!strncmp(input, "initialize", 10))
		{
			fcurr = head;
			dcurr = NULL;
			ecurr = NULL;
			mcurr = NULL;
			wcurr = NULL;
			iq_curr = NULL;
			iq_head = NULL;
			iq_tail = NULL;
			X = '\0';
			for (i = 0; i < 32; i++)
			{
				r[i] = 0;
				valid[i] = 1;
			}
			for (i = 0; i < 1000; i++)
			{
				valid_address[i] = 1;
				address[i] = '\0';
				p_reg[i].valid = 1;
			}
			printf("\nInitialized\n");
		}
		else if (!strncmp(input, "simulate", 8))
		{
			int n, status;
			printf("\nIn simulate\n");
			strcat(input, " ");
			const char space[2] = " ";
			instr1 = strtok(input, space);
			instr2 = strtok(NULL, space);
			if (instr2 != NULL)
			{
				instr2[strlen(instr2) - 1] = '\0';
				n = atoi(instr2);
				printf("\nRunning for |%s| of |%d| Cycles\n", instr2, n);
				for (j = 1; j <= n; j++)
				{
					wb();
					mem();
					exec();
					decode();
					status=fetch(fcurr);
					if (status == 1)
					{
						fcurr = fcurr->next;
					}
				}
			}
			else { printf("\nPlease enter the command in the format simulate <number of cycles>"); }
		}
		else if (!strncmp(input, "display", 7))
		{
			printf("\nRegister Values \n");
			int i;
			printf("\n***********************************************************************************************************************************\n");
			for (i = 0; i < 32; i++)
			{
				if (!(i % 8))
				{
					printf("\n");
				}
				printf("R[%02d]=%02d ", i, r[i]);
			}
			printf("\n***********************************************************************************************************************************\n");
			printf("\nValues in each stage currently:\n");
			printf("\nInstruction Fetch : %s", fcurr->instr);
			printf("\nInstruction Decode : %s", dcurr->instr);
			if (ecurr_int != NULL) {
				int l = ecurr_int->pc;
				struct list *temp = head;
				while (temp->pos != l)
				{
					temp = temp->next;
				}
				printf("\nInstruction in INT FU :%s", temp->instr);
			}
			if (!temp)
			{
				printf("\nInstruction in INT FU :%s", NULL);
			}
			if (ecurr_lsq != NULL) {
				int l = ecurr_lsq->pc;
				struct list *temp = head;
				while (temp->pos != l)
				{
					temp = temp->next;
				}
				printf("\nInstruction in LSQ FU :%s", temp->instr);
			}
			if (!temp)
			{
				printf("\nInstruction in LSQ :%s", NULL);
			}
			if (ecurr_mul != NULL) {
				int l = ecurr_mul->pc;
				struct list *temp = head;
				while (temp->pos != l)
				{
					temp = temp->next;
				}
				printf("\nInstruction in MUL FU :%s", temp->instr);
			}
			if (!temp)
			{
				printf("\nInstruction in MUL FU :%s", NULL);
			}
			if (mcurr != NULL) {
				int l = mcurr->pc;
				struct list *temp = head;
				while (temp->pos != l)
				{
					temp = temp->next;
				}
				printf("\nInstruction in Memory :%s", temp->instr);
			}
			if (!temp)
			{
				printf("\nInstruction in Memory :%s", NULL);
			}
			if (wcurr != NULL) {
				int l = wcurr->pc;
				struct list *temp = head;
				while (temp->pos != l)
				{
					temp = temp->next;
				}
				printf("\nInstruction in WB :%s", temp->instr);
			}
			if(!temp)
			{
				printf("\nInstruction in WB :%s", NULL);
			}
			//printf("\nInstruction Execute : %s", ecurr->instr);
			//printf("\nInstruction Memory : %s", mcurr->instr);
			//printf("\nInstruction Write Back : %s", wcurr->instr);
			printf("\n***********************************************************************************************************************************\n");
			printf("Memory values:\n");
			for (i = 0; i < 100; i++)
			{
				if (!(i % 10))
				{
					printf("\n");
				}
				printf("Mem[%02d]=%02d ", i, address[i]);
			}
			printf("\n***********************************************************************************************************************************\n");
		}
		free(input);
	}
	return 0;
}
