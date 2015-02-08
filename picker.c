//picker.c
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include "rand.h"
#include "bip39_english.h"

const size_t max_word_len = 8;

typedef struct _letterpos_filter
{
	size_t m_lett_ndx;
	char m_letter;
}letterpos_filter;

typedef struct _letterpos_filter_set
{
	size_t m_cnt;
	letterpos_filter m_filters[8];
}letterpos_filter_set;
typedef struct _words_list
{
	size_t m_sz;
	const char *m_wl[2048];
}words_list;
typedef struct _mnemonic_word_discover
{
	int m_is_real_word;
	size_t m_fake_word_global_index;// ignored for real words
	size_t m_word_position_in_phrase;// ignored for fake words
	letterpos_filter_set m_filters;
	words_list m_remaining_words;
}mnemonic_word_discover;
typedef struct _mnemonic_phrase_discover
{
	size_t m_actual_word_count; // number of real words in the mnemonic
	size_t m_total_word_count; // sum of real words and fake words counts
	mnemonic_word_discover m_words[24];
}mnemonic_phrase_discover;

void do_letterpos_filter( letterpos_filter *f, words_list *wdls )
{
	size_t output_wl_ndx=0;
	size_t at=0;
	for(; at< wdls->m_sz; ++at)
	{
		size_t thisword_len = strlen(wdls->m_wl[at]);
		if(thisword_len <= f->m_lett_ndx || // passed the end of the word you cannot filter
			wdls->m_wl[at][f->m_lett_ndx] == f->m_letter )
		{
			wdls->m_wl[output_wl_ndx] = wdls->m_wl[at];
			++ output_wl_ndx;
		}
	}
	wdls->m_sz = output_wl_ndx;
}

int append_new_letterpos_filter( letterpos_filter *newf,  mnemonic_word_discover *to_this )
{
	if( to_this->m_filters.m_cnt >= max_word_len )
		return 0;//no more filters than max size of a word make sense
	to_this->m_filters.m_filters[to_this->m_filters.m_cnt] = *newf;
	to_this->m_filters.m_cnt ++;
	do_letterpos_filter( newf, &to_this->m_remaining_words);
	return 1;
}
size_t select_next_letterpos( mnemonic_word_discover *for_this )
{
	//find a letter ndx that has not been used yet
	size_t max = max_word_len;
	if( for_this->m_filters.m_cnt < 3 )
		max = 3; //prefer the first 3 letters, dont go beyond that unless we have to
	while( 1 )
	{
		//find an unused index
		uint8_t data;
		size_t checkk;
		int found_conflict=0;
		random_buffer(&data, 1);
		data = data % max;
		for( checkk=0; checkk < for_this->m_filters.m_cnt; ++ checkk)
		{
			if( for_this->m_filters.m_filters[checkk].m_lett_ndx == data )
			{
				found_conflict=1;
				break;
			}
		}
		if( found_conflict )
			continue;
		//else, add new filter details
		return (size_t) data; 
	}
}
//if done returns -1
//else returns which ndx needs more reduction
int next_query_select( mnemonic_phrase_discover *for_this, size_t max_choices_remaining, size_t *pdone )
{
	size_t avail[24];
	size_t avail_cnt=0;
	size_t at;
	for( at = 0; at<  24; ++ at)
	{
		if( for_this->m_words[at].m_remaining_words.m_sz > max_choices_remaining )
		{
			avail[avail_cnt] = at;
			avail_cnt ++; 
		}
	}
	if(!avail_cnt)
		return -1;
	uint8_t data;
	random_buffer(&data, 1);
	//fprintf(stderr,"%3zu%% complete\n", (24-avail_cnt) * 100 / 24);
	*pdone = (24-avail_cnt) * 100 / 24;
	return avail[data % avail_cnt];
}

char input_line[2018];
void get_user_input(const char *q )
{
	fprintf(stderr,"%s\n", q );
	fgets(input_line,2048,stdin);
}

const char * ordinal_position_name(size_t ord, char*retb)
{
	++ord;
	if(ord > 99) ord = 99;
	size_t offs=0;
	if(ord > 9 )
	{
		retb[0] = '0' + (ord / 10);
		++offs;
	}
	size_t modu = ord%10;
	if(modu == 1 && ord != 11 )
	{
		retb[offs+1] = 's';
		retb[offs+2] = 't';
	}
	else if(modu == 2 && ord != 12 )
	{
		retb[offs+1] = 'n';
		retb[offs+2] = 'd';
	}
	else if(modu == 3 )
	{
		retb[offs+1] = 'r';
		retb[offs+2] = 'd';
	}
	else
	{
		retb[offs+1] = 't';
		retb[offs+2] = 'h';
	}
	retb[offs] = '0' + modu;
	retb[offs+3] = 0;
	return retb;
}

void setup_workset( size_t n_words, size_t min_words, mnemonic_phrase_discover *workset)
{
	const char **g_wl = wordlist;
	workset->m_actual_word_count = n_words;
	workset->m_total_word_count = n_words;
	if( n_words < min_words )
		workset->m_total_word_count = min_words;
	size_t next;
	for( next=0; next < workset->m_actual_word_count ; ++next )
	{
		//initialize real words
		workset->m_words[ next ].m_is_real_word = 1;
		workset->m_words[ next ].m_word_position_in_phrase= next;
	}
	for( next=workset->m_actual_word_count; next < min_words; ++next )
	{
		//initialize fake words
		workset->m_words[ next ].m_is_real_word = 0;

		uint16_t r;
		random_buffer((uint8_t*)&r, sizeof(r));
		r &= 0x7FF;
		workset->m_words[ next ].m_fake_word_global_index=r;
	}

	for( next=0 ; next < workset->m_total_word_count; ++next )
	{
		size_t k;
		for( k=0;k<2048; ++k) 
			workset->m_words[ next ].m_remaining_words.m_wl[k]=g_wl[k];
		workset->m_words[ next ].m_remaining_words.m_sz=2048;
	}
	//workset is now ready to go; it contains workset->m_total_word_count ordered workets
}
size_t average_remaining_num_choices_for_trezor(mnemonic_phrase_discover *workset)
{
	if( workset->m_actual_word_count < 1 )
		return 0;
	size_t sum=0;
	size_t next;
	for( next=0; next < workset->m_total_word_count; ++next )
	{
		if(workset->m_words[next].m_is_real_word )
		{
			sum += workset->m_words[next].m_remaining_words.m_sz;
		}
	}
	return sum / workset->m_actual_word_count;
}
void dump_remaining_choices_for_trezor(mnemonic_phrase_discover *workset)
{
	//now each position should have at most max_select words possibilities
	size_t next;
	for( next=0; next < workset->m_total_word_count; ++next )
	{
		if(workset->m_words[next].m_is_real_word )
		{
			fprintf(stdout,"Word %zu\n\t", next + 1);
			size_t atw;
			for( atw=0; atw < workset->m_words[next].m_remaining_words.m_sz; ++atw )
			{
				fprintf(stdout," \"%s\",", workset->m_words[next].m_remaining_words.m_wl[atw]);
			}
			fprintf(stdout,"\n");
		}
	}
}
int interactive(mnemonic_phrase_discover *workset, size_t max_select, size_t min_words )
{
	const char **g_wl = wordlist;
	// get letters form the user until 24 words have been reduced to at most max_select choices
	get_user_input("how many words? 12 18 24");

	setup_workset( (size_t)atol( input_line), min_words, workset );

	int next_q;
	char inputs[1024];
	size_t input_cnt=0;
	size_t pdone=0;
	while( -1 != (next_q=next_query_select(workset,max_select,&pdone)) )
	{
		fprintf(stderr,"%zu inputs so far, %3zu%% done\n", input_cnt, pdone);
		const char * fakeword;
		size_t pos = select_next_letterpos( &workset->m_words[next_q]);
		if( workset->m_words[next_q].m_is_real_word )
		{
			char retb1[5],retb2[5];
			//prompt the user for the letter at pos of word at workset->m_words[next_q].m_word_position_in_phrase
			fprintf(stderr,"Please enter the %s letter of the %s word of the phrase", 
					ordinal_position_name(pos,retb1), ordinal_position_name(next_q,retb2) );
		}
		else
		{
			char retb1[5];
			//prompt for letter of fake word:
			fakeword = g_wl[workset->m_words[next_q].m_fake_word_global_index];
			fprintf(stderr,"Please enter the %s letter of the word \'%s\'", 
					ordinal_position_name(pos,retb1), fakeword );
		}
		get_user_input("");
		char letter_input = input_line[0];
		inputs[input_cnt++]=letter_input;
		inputs[input_cnt+1]=0;

		if( !letter_input || 
				( ! workset->m_words[next_q].m_is_real_word && 
				  letter_input != fakeword[pos]
				  ))
		{
			fprintf(stderr,"Invalid input\n");
			return 1;
		}

		if( letter_input != 0)
		{
			letterpos_filter newf;
			newf.m_lett_ndx = pos;
			newf.m_letter = letter_input;
			append_new_letterpos_filter(&newf, &workset->m_words[next_q]);
			if( workset->m_words[next_q].m_remaining_words.m_sz < 1 )
			{
				fprintf(stderr,"Invalid input\n");
				return 1;
			}
		}
	}

	fprintf( stdout, "You made %zu inputs:\n%s\nThe remaining words can be worked out by the device\n", 
			input_cnt, inputs );
	dump_remaining_choices_for_trezor(workset);
		
	return 0;
}

void parse_words( char *buf, size_t *cnt, const char *wrds[] )
{
	const char **g_wl = wordlist;
	*cnt=0;
	while( 1 )
	{
		const char *start = buf;
		while( *buf && *buf != ' ' && *buf != '\n')
			++buf;

		char ended_at = *buf;
		*buf = 0;

		if(buf > start)
		{
			size_t atw1;
			for( atw1 = 0; atw1 < 2048 ;++atw1 )
			{
				if(0 == strcmp(start,g_wl[atw1]) )
					break;
			}
			if( atw1 >= 2048 )
				break;//invalid word found

			wrds[*cnt]=start;
			++*cnt;
		}

		if( ended_at == 0 || *cnt >= 24 )
			break;

		++buf;
	}
}

void non_interactive_solve(mnemonic_phrase_discover *workset, size_t *inp_cnt, char *inps, 
		const char *words[], size_t max_select )
{
	const char **g_wl = wordlist;

	int next_q;
	size_t pdone=0;
	while( -1 != (next_q=next_query_select(workset,max_select,&pdone)) )
	{
		const char *word;
		size_t pos = select_next_letterpos( &workset->m_words[next_q]);
		if( workset->m_words[next_q].m_is_real_word )
			word = words[next_q];
		else
			word = g_wl[workset->m_words[next_q].m_fake_word_global_index];

//fprintf(stderr,"%s %zu\n", word, pos );

		char letter_input = word[pos];
		inps[(*inp_cnt)++] = letter_input;
		inps[ (*inp_cnt) ] = 0;

		letterpos_filter newf;
		newf.m_lett_ndx = pos;
		newf.m_letter = letter_input;
		append_new_letterpos_filter(&newf, &workset->m_words[next_q]);
	}

}

void chomp( char * wordy_orig)
{
	while( *wordy_orig )
	{
		if(*wordy_orig == '\n')
		{
			*wordy_orig = 0;
			break;
		}
		++ wordy_orig;
	}
}

int main( int argc, char *argv[] )
{
	init_rand();
	int do_unit = 0;
	int do_verbose = 0;
	int output_choices_only=0;
	size_t max_select = 10;
	size_t min_words = 24;
	int opt;
	while ((opt = getopt(argc, argv, "cvus:m:")) != -1)
	{
		switch (opt)
		{
			case 'c':
				output_choices_only=1;
				break;
			case 'v':
				do_verbose++;
				break;
			case 'u':
				do_unit = 1;
				break;
			case 's':
				max_select = (size_t)strtol(optarg, 0, 0);
				break;
			case 'm':
				min_words = (size_t)strtol(optarg, 0, 0);
				break;
			default:
				break;
		}
	}
	argc-=optind;
	argv+=optind;

	mnemonic_phrase_discover workset;
	memset(&workset,0,sizeof(mnemonic_phrase_discover));


	if(do_unit)
	{
		int line=0;
		while(!feof(stdin) )
		{
			//read a line of input
			char wordy_orig[2048];
			char wordy[2048];
			if( ! fgets(wordy,2048,stdin) )
				break;
			++line;
			//setup a workset of the right size
			size_t nwords;
			const char *words[24];
			memcpy(wordy_orig,wordy,2048);
			chomp(wordy_orig);
			parse_words(wordy,&nwords,words);
			if( nwords != 12 && nwords != 18 && nwords != 24 )
			{
				fprintf(stderr,"Input line %d is invalid\n", line);
			}
			setup_workset( nwords, min_words, &workset );
			//perform the solution inputs required as a user would
			size_t inpcnt = 0;
			char inputs[1024];
			non_interactive_solve( &workset, &inpcnt, inputs, words, max_select);	

			if(output_choices_only)
			{
				fprintf(stdout, "%s\n",inputs);
			}
			else if(do_verbose)
			{
				//print a summary
				size_t choiceleft = average_remaining_num_choices_for_trezor(&workset);
				fprintf(stdout, "Line %d : %s\n\tuser input: %s (%zu pc interactions) (%zu trzr ave choices)\n",
						line, wordy_orig, inputs, inpcnt, choiceleft );
				if(do_verbose> 1) 
					dump_remaining_choices_for_trezor(&workset);
			}
			else
			{
				fprintf(stdout, "%zu\n", inpcnt);
			}
			//reset the object
			memset(&workset,0,sizeof(mnemonic_phrase_discover));
		}
	}
	else
	{
		interactive(&workset,max_select, min_words);
	}


	finalize_rand();
	return 0;
}
