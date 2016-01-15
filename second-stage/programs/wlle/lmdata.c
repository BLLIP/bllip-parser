/*
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.  You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

/* lmdata.c
 *
 * Mark Johnson, 11th April 2005, last modified 31st July 2008
 *
 * This is a version of data.c with additions for pairwise loss functions.
 */

#include "lmdata.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CALLOC(n, s)	calloc(n, s)
#define MALLOC(n)	malloc(n)
#define REALLOC(x,n)	realloc(x,n)
#define FREE(x)		free(x)

/* used on data never freed */

#define SMALLOC(n)		blockalloc_malloc(n)
#define SMCOPY(x,n)		blockalloc_copy(x,n)
#define SREALLOC(x,old_n,new_n)	blockalloc_realloc(x,old_n,new_n)

/***********************************************************************
 *                                                                     *
 *                             Utility                                 *
 *                                                                     *
 ***********************************************************************/

__inline__
DataFloat feature_value(parse_type *p, feature_type f) {
  size_type i;
  for (i = 0; i < p->nfc; ++i)
    if (p->fc[i].f == f)
      return p->fc[i].c;
  for (i = 0; i < p->nf; ++i)
    if (p->f[i] == f)
      return 1;
  return 0;
}  // feature_value

/*! parse_score() returns the score for parse *p. */

__inline__
Float parse_score(const parse_type *p, const Float w[]) {
  size_type i;
  Float score = 0;
  for (i = 0; i < p->nf; ++i)
    score += w[p->f[i]];
  for (i = 0; i < p->nfc; ++i)
    score += p->fc[i].c * w[p->fc[i].f];
  return score;
}  /* parse_score() */

/*! sentence_scores() loads score[] with the scores of all parses in s,
 *! and sets max_correct_score and max_score.  Returns the index of the
 *! last highest scoring parse.
 */

#ifndef __clang__
__inline__
#endif
void sentence_scores(sentence_type *s, const Float w[], Float score[],
		     Float *best_correct_score, int *best_correct_i,
		     Float *best_score, int *best_i) {
  size_type i;
  Float sc;
  assert(s->nparses > 0);

  *best_i = 0; 
  *best_correct_i = -1;
  *best_correct_score = 0;

  *best_score = score[0] = sc = parse_score(&s->parse[0], w);
  if (s->parse[0].Pyx > 0) {
    *best_correct_i = 0;
    *best_correct_score = sc;
  }

  for (i = 1; i < s->nparses; ++i) {
    score[i] = sc = parse_score(&s->parse[i], w);
    if (sc >= *best_score) {
      *best_i = i;
      *best_score = sc;
    }
    if (s->parse[i].Pyx > 0 
	&& (*best_correct_i < 0 || sc > *best_correct_score)) {
      *best_correct_i = i;
      *best_correct_score = sc;
    }
  }

  assert(finite(*best_score));
  assert(s->Px == 0 || *best_correct_i >= 0);
  if (*best_correct_score > *best_score) {
    fprintf(stderr, "## best_correct_score = %g, best_score = %g\n## score =",
	    *best_correct_score, *best_score);
    for (i = 0; i < s->nparses; ++i)
      fprintf(stderr, " %g", score[i]);
    fprintf(stderr, "\n");
  }
  assert(s->Px == 0 || *best_correct_score <= *best_score);
}  /* sentence_scores() */

/*! max_score_index() returns the index of the parse with the highest score. */

size_type max_score_index(const sentence_type *s, const Float w[]) {
  size_type i, max_i = 0;
  Float max_score = parse_score(&s->parse[0], w);
  assert(finite(max_score));

  if (s->nparses <= 1)
    return 0;

  for (i = 1; i < s->nparses; ++i) {
    Float score = parse_score(&s->parse[i], w);
    assert(finite(score));
    if (score > max_score) {
      max_i = i;
      max_score = score;
    }
  }
  return max_i;
}  /* max_score_index */

/* memory allocation stuff */

#define NUNITS(N)    (1 + (N-1)/sizeof(align))

typedef int align;

/* blocksize is the number of align-sized units in the memory
 * block grabbed from malloc.  This setting will give approximately
 * 64Mb-sized blocks.  I subtract 16 align-sized units from exactly
 * 64Mb so that the block with my and the system's overhead will
 * probably still fit in a real 64Mb-size unit.
 */

#define BLOCKSIZE    (16777216-16)

struct blockalloc {
  align           data[BLOCKSIZE];
  size_t          top;
  struct blockalloc *next;
};

static struct blockalloc *current_block = NULL;

void blockalloc_free_all(void)
{
  struct blockalloc *p = current_block;

  while (p) {
    struct blockalloc *q = p->next;
    FREE(p);
    p = q;
  }

  current_block = NULL;
}

__inline__
static void* blockalloc_malloc(size_t n_char)
{
  struct blockalloc *p = current_block;
  size_t n;

  if (n_char == 0)
    return NULL;

  n = NUNITS(n_char);

  if (n > BLOCKSIZE)
    return NULL;

  if (p == NULL || p->top < n) {
    p = MALLOC(sizeof(struct blockalloc));
    if (p == NULL) {
      FILE* fp = fopen("/proc/self/stat", "r");
      assert(fp);
      int utime;
      int stime;
      unsigned int vsize;
      unsigned int rss;
      int result = 
	fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %d %d %*d %*d %*d %*d"
	       "%*u %*u %*d %u %u", &utime, &stime, &vsize, &rss);
      assert(result == 4);
      fclose(fp);
      fprintf(stderr, "## Error: failed to allocate memory.  utime = %d, vsize = %u\n", utime, vsize);
      exit(EXIT_FAILURE);
    }
    p->next = current_block;
    p->top = BLOCKSIZE;
    current_block = p;
  }

  p->top -= n;
  return (void *) &p->data[p->top];
}  // blockalloc_malloc()

__inline__
static void* blockalloc_copy(void *p, size_t n)
{
  void *new_p = SMALLOC(n);
  if (new_p == NULL)
    return NULL;
  if (n != 0)
    memcpy(new_p, p, n);
  return new_p;
}

__inline__
static void* blockalloc_realloc(void *p, size_t old_n_char, size_t new_n_char)
{
  void *new_p = SMALLOC(new_n_char);
  size_t n_move = old_n_char < new_n_char ? old_n_char : new_n_char;
  if (new_p == NULL)
    return NULL;
  if (n_move != 0)
    memcpy(new_p, p, n_move);
  FREE(p);
  return new_p;
}

/***********************************************************************
 *                                                                     *
 *                            read_corpus()                            *
 *                                                                     *
 ***********************************************************************/

#define MIN_NF   1024
#define MIN_NFC  1024

/* static data for read_parse() */

static size_type read_parse_nf_max = 0;
static size_type read_parse_nfc_max = 0;
static feature_type *read_parse_fp = NULL;
static fc_type *read_parse_fcp = NULL;

static void read_parse(FILE *in, parse_type *p, feature_type *fmax) {
  feature_type f;
  int nread;
  size_type nf = 0, nfc = 0;
  DataFloat c;

  assert(read_parse_fp != NULL);
  assert(read_parse_fcp != NULL);

  p->p = 1;
  nread = fscanf(in, " P = " DATAFLOAT_FORMAT " ", &p->p); /* read p */
  assert(nread != EOF);

  p->w = 0;
  nread = fscanf(in, " W = " DATAFLOAT_FORMAT " ", &p->w); /* read pwinner */
  assert(nread != EOF);

  while ((nread = fscanf(in, " " FEATURE_FORMAT " ", &f))) { /* read feature */
    assert(nread != EOF);
    if (nread == 0)
      break;
    assert(f >= 0);

    c = 1.0;                                          /* default value for c */
    /* read feature count */
    nread = fscanf(in, " = " DATAFLOAT_FORMAT " ", &c); 
    assert(nread != EOF);

    if (f > *fmax)
      *fmax = f;

    if (c == 1.0) {
      if (nf >= read_parse_nf_max) {
	read_parse_nf_max *= 2;
	read_parse_fp = REALLOC(read_parse_fp, 
				read_parse_nf_max*sizeof(feature_type));
	assert(read_parse_fp != NULL);
      }
      assert(nf<read_parse_nf_max);
      read_parse_fp[nf] = f;
      nf++;
    }
    else {   /* c != 1 */
      if (nfc >= read_parse_nfc_max) {
	read_parse_nfc_max *= 2;
	read_parse_fcp = REALLOC(read_parse_fcp, 
				 read_parse_nfc_max*sizeof(fc_type));
	assert(read_parse_fcp != NULL);
      }
      assert(nfc<read_parse_nfc_max);
      read_parse_fcp[nfc].f = f;
      read_parse_fcp[nfc].c = c;
      nfc++;
    }
  }

  /* copy features into p */

  p->nf = nf;
  p->f = SMCOPY(read_parse_fp, nf*sizeof(feature_type));
  if (nf > 0)
    assert(p->f != NULL);

  p->nfc = nfc;
  p->fc = SMCOPY(read_parse_fcp, nfc*sizeof(fc_type));
  if (nfc > 0)
    assert(p->fc != NULL);

  fscanf(in, " ,");   /* read final ',' */

}  /* read_parse() */


/*! read_sentence() reads a sentence, normalizes pwinner on parses,
 *!  and updates maxnparses.
 */

int read_sentence(corpusflags_type *flags, FILE *in, sentence_type *s, 
		  feature_type *fmax, int *maxnparses) {
  int i, nread, best_fscore_index = -1, nwinners = 0;
  DataFloat fscore, best_logprob = -DATAFLOAT_MAX, best_fscore = -1;

  nread = fscanf(in, " G = " DATAFLOAT_FORMAT " ", &s->g);
  if (nread == EOF)
    return EOF;
  assert(nread == 0 || nread == 1);
  if (nread == 0)
    s->g = 1;

  nread = fscanf(in, " N = %d", &s->nparses);
  if (nread == EOF)
    return EOF;
  assert(nread == 1);
  assert(s->nparses >= 0);
  if (s->nparses > *maxnparses)
    *maxnparses = s->nparses;

  if (s->nparses > 0) {
    s->parse = SMALLOC(s->nparses*sizeof(parse_type));
    assert(s->parse != NULL);
  }
  else
    s->parse = NULL;

  for (i = 0; i < s->nparses; ++i) {
    read_parse(in, &s->parse[i], fmax);
    // handle the admittedly strange case where a parse has no brackets
    if (s->parse[i].w == 0.0 || s->parse[i].p == 0.0) {
      fscore = 0.0;
    } else {
      fscore = 2 * s->parse[i].w/(s->parse[i].p+s->g);
    }
    if (fscore+DATAFLOAT_EPS >= best_fscore) {
      Float logprob = feature_value(&s->parse[i], 0); // logprob is feature 0
      if (fabs(fscore-best_fscore) < 2*DATAFLOAT_EPS) {
	++nwinners;                   // tied best f-scores    
	if (logprob > best_logprob) { // pick candidate with best logprob
	  best_fscore = fscore;
	  best_logprob = logprob;
	  best_fscore_index = i;
	}
      }
      else {
	best_fscore = fscore;
	best_logprob = logprob;
	best_fscore_index = i;
	nwinners = 1;
      }
    }
  }

  // skip sentences where the best f-score was 0 since there's nothing to learn from them
  if (best_fscore == 0.0) {
    best_fscore = -1;
    best_logprob = -DATAFLOAT_MAX;
    best_fscore_index = -1;
    nwinners = 0;
  }
  if (best_fscore_index >= 0) {  /* is there a winner? */
    Float sum_Pyx = 0;
    assert(nwinners > 0);
    assert(best_fscore_index < s->nparses);
    s->Px = 1.0;		 /* indicate that there is a winner */
    s->correct_index = best_fscore_index;
    if (flags && flags->Pyx_factor > 1) {
      Float Z = 0;
      for (i = 0; i < s->nparses; ++i) {
	fscore = 2 * s->parse[i].w/(s->parse[i].p+s->g);
	Z += pow(flags->Pyx_factor, fscore - best_fscore);
      }
      for (i = 0; i < s->nparses; ++i) {
	fscore = 2 * s->parse[i].w/(s->parse[i].p+s->g);
	sum_Pyx += s->parse[i].Pyx 
	  = pow(flags->Pyx_factor, fscore - best_fscore) / Z;
      }
    }
    else if (flags && flags->Pyx_factor > 0) { 
      /* Pyx_factor == 1; all winners get equal Pyx */
      for (i = 0; i < s->nparses; ++i) {
	Float fscore = 2 * s->parse[i].w/(s->parse[i].p+s->g);
	if (fabs(best_fscore-fscore) < 2*DATAFLOAT_EPS)
	  sum_Pyx += s->parse[i].Pyx = 1.0/nwinners;
	else
	  sum_Pyx += s->parse[i].Pyx = 0;
      }
    }	  
    else {  /* Pyx_factor == 0; f-score winner gets Pyx = 1, all others 0 */
      for (i = 0; i < s->nparses; ++i)  
	sum_Pyx += s->parse[i].Pyx = (i == best_fscore_index);
    }
    assert(fabs(sum_Pyx - 1) <= DATAFLOAT_EPS);
  }
  else {    /* no winner, set Px to 0.0 to indicate this */
    s->Px = 0.0;
    s->correct_index = s->nparses;
    for (i = 0; i < s->nparses; ++i)
      s->parse[i].Pyx = 0.0;
    /* We used to ensure that s->nparses == 0 here, but 
       instead do nothing since read_corpus now knows to
       skip these sentences.  These sentences show up
       when we have parse failures and the parser returns
       a flat bracketing. */
  }

  return s->nparses;
}  /* read_sentence() */

corpus_type *read_corpus(corpusflags_type *flags, FILE *in) {
  sentence_type s;
  feature_type fmax = 0;
  int nread, i = 0, maxnparses = 0, nloserparses = 0;
  Float sum_g = 0;

  /* allocate feature counts */
  read_parse_nfc_max = MIN_NFC;
  read_parse_fcp = MALLOC(read_parse_nfc_max*sizeof(fc_type));
  assert(read_parse_fcp != NULL);
  /* allocate features w/ 1 count */
  read_parse_nf_max = MIN_NF;
  read_parse_fp = MALLOC(read_parse_nf_max*sizeof(feature_type)); 
  assert(read_parse_fp != NULL);

  corpus_type *c = SMALLOC(sizeof(corpus_type));
  assert(c != NULL);

  size_type nsentences;
  nread = fscanf(in, " S = %d ", &nsentences);
  assert(nread != EOF);
  c->sentence = MALLOC(nsentences*sizeof(sentence_type));
  assert(c->sentence != NULL);
 
  while (read_sentence(flags, in, &s, &fmax, &maxnparses) != EOF) {
    if (i >= nsentences) {
      nsentences *= 2;
      c->sentence = REALLOC(c->sentence, nsentences*sizeof(sentence_type));
      assert(c->sentence != NULL);
    }
    assert(i < nsentences);

    /* skip sentences with no winners but some parses -- these are
       typically parse failures. */
    if (s.Px == 0.0 && s.nparses != 0)
      continue;

    c->sentence[i++] = s;
    sum_g += s.g;
    if (s.Px > 0)
      nloserparses += s.nparses - 1;
  }
  c->nsentences = i;
  c->sentence = SREALLOC(c->sentence, nsentences*sizeof(sentence_type),
			 c->nsentences*sizeof(sentence_type));
  assert(c->sentence != NULL);
  c->nfeatures = fmax+1;
  c->maxnparses = maxnparses;
  c->nloserparses = nloserparses;

  if (flags && flags->Px_propto_g)
    for (i = 0; i < c->nsentences; ++i)  /* normalize Px */
      c->sentence[i].Px *= c->nsentences * c->sentence[i].g / sum_g;

  FREE(read_parse_fcp);
  FREE(read_parse_fp);

  return c;
}  /* read_corpus() */

corpus_type *read_corpus_file(corpusflags_type *flags, const char* filename) {
  const char* filesuffix = strrchr(filename, '.');
  char *command = NULL;
  FILE *in;
  corpus_type *corpus;
  if (strcasecmp(filesuffix, ".bz2") == 0) {
    const char bzcat[] = "bzcat ";
    command = malloc(sizeof(bzcat)+strlen(filename)+1);
    strcpy(command, bzcat);
    strcat(command, filename);
    in = popen(command, "r");
  }
  else if (strcasecmp(filesuffix, ".gz") == 0) {
    const char zcat[] = "gunzip -c ";
    command = malloc(sizeof(zcat)+strlen(filename)+1);
    strcpy(command, zcat);
    strcat(command, filename);
    in = popen(command, "r");
  }
  else
    in = fopen(filename, "r");
  if (in == NULL) {
    if (command == NULL) 
      fprintf(stderr, "## Error: couldn't open corpus file %s\n", filename);
    else
      fprintf(stderr, "## Error: couldn't popen command %s\n", command);
    exit(EXIT_FAILURE);
  }
  corpus = read_corpus(flags, in);
  if (command != NULL) {
    free(command);
    pclose(in);
  }
  else
    fclose(in);
  return corpus;
}  /* read_corpus_file() */

/***********************************************************************
 *                                                                     *
 *                      linear logistic regression                     *
 *                                                                     *
 *                            corpus_stats()                           *
 *                                                                     *
 ***********************************************************************/

/*! sentence_stats() returns - log P~(x) (E_P~[w.f|x] - log Z_w(x))
 *!  increments E_Ew[f] with the difference of the expected 
 *!  value of f for the winners and for all parses,
 *!  and increments the precision/recall scores.
 */

Float sentence_stats(sentence_type *s, const Float w[], Float score[], Float E_Ew[],
		     Float *sum_g, Float *sum_p, Float *sum_w) {
  Float best_correct_score, best_score;
  int i, j, best_i, best_correct_i;
  Float Z = 0, logZ, Ecorrect_score = 0;

  *sum_g += s->g;

  if (s->nparses <= 0) 
    return 0;

  sentence_scores(s, w, score, &best_correct_score, &best_correct_i, &best_score, &best_i);
  *sum_p += s->parse[best_i].p;
  *sum_w += s->parse[best_i].w;
  
  if (s->Px == 0)  /* skip statistics calculation if Px == 0 */
    return 0;

  assert(best_correct_score <= best_score);

  for (i = 0; i < s->nparses; ++i) {   /* compute Z and Zw */
    assert(score[i] <= best_score);
    Z += exp(score[i] - best_score);
    assert(finite(Z));
    if (s->parse[i].Pyx > 0) 
      Ecorrect_score += s->parse[i].Pyx * score[i];
  }

  logZ = log(Z) + best_score;

  /* calculate expectations */

  for (i = 0; i < s->nparses; ++i) {
    Float cp = exp(score[i] - logZ);  /* P_w(y|x) */
    assert(finite(cp));

    if (s->parse[i].Pyx > 0)  /* P_e(y|x)  */
      cp -= s->parse[i].Pyx;

    assert(cp >= -1.0);
    assert(cp <= 1.0);
    
    cp *= s->Px;

    /* calculate expectations */

    for (j = 0; j < s->parse[i].nf; ++j)  /* features with 1 count */
      E_Ew[s->parse[i].f[j]] += cp;

    for (j = 0; j < s->parse[i].nfc; ++j) /* features with arbitrary counts */
      E_Ew[s->parse[i].fc[j].f] += cp * s->parse[i].fc[j].c;
  }
  return - s->Px * (Ecorrect_score - logZ);
}  /* sentence_stats() */
  

/* corpus_stats() returns - count * log P(winners|parses), 
 *  increments E_Ew[f] with the difference of the expected 
 *  value of f for all parses and just for the winners
 *  (i.e., the derivative of - count * log P(winners|parses)),
 *  and increments ncorrect.
 */

Float corpus_stats(corpus_type *c, const Float w[], Float E_Ew[], 
		   Float *sum_g, Float *sum_p, Float *sum_w) 
{
  Float neglogP = 0;
  Float *score = MALLOC(c->maxnparses*sizeof(Float));
  int i;

  assert(score != NULL);

  *sum_g = *sum_p = *sum_w = 0;          /* zero precision/recall counters */
  for (i = 0; i < c->nfeatures; ++i)     /* zero Ew_E[] */
    E_Ew[i] = 0;
  for (i = 0; i < c->nsentences; ++i)    /* collect stats from sentences */
    neglogP += sentence_stats(&c->sentence[i], w, score, E_Ew, 
			      sum_g, sum_p, sum_w);
  FREE(score);
  return neglogP;
}  /* corpus_stats() */


/***********************************************************************
 *                                                                     *
 *                          EM-style log loss                          *
 *                                                                     *
 ***********************************************************************/

/*! emll_sentence_stats() returns the EM-like log loss fn - P~(x)[ log E_w[P~|x] ]
 *!  increments dL_dw[f] with its derivative,
 *!  and increments the precision/recall scores.
 */

Float emll_sentence_stats(sentence_type *s, const Float w[], 
			  Float score[], Float dL_dw[], 
			  Float *sum_g, Float *sum_p, Float *sum_w) {
  Float best_correct_score, best_score;
  int i, j, best_i, best_correct_i;
  Float Z = 0, logZ;    /*!< Z is the partition fn calculated over all parses */
  Float Zc = 0, logZc;  /*!< Zc is the partition fn calculated over correct parses */

  *sum_g += s->g;

  if (s->nparses <= 0) 
    return 0;

  sentence_scores(s, w, score, &best_correct_score, &best_correct_i, &best_score, &best_i);
  *sum_p += s->parse[best_i].p;
  *sum_w += s->parse[best_i].w;
  
  if (s->Px == 0)  /* skip statistics calculation if Px == 0 */
    return 0;

  assert(best_correct_score <= best_score);

  for (i = 0; i < s->nparses; ++i) {   /* compute Z and Zc */
    assert(score[i] <= best_score);
    Z += exp(score[i] - best_score);
    assert(finite(Z));
    if (s->parse[i].Pyx > 0) {
      assert(score[i] <= best_correct_score);
      Zc += s->parse[i].Pyx * exp(score[i] - best_correct_score);
      assert(finite(Zc));
    }
  }

  logZ = log(Z) + best_score;
  logZc = log(Zc) + best_correct_score;

  /* calculate expectations */

  for (i = 0; i < s->nparses; ++i) {
    Float cp = exp(score[i] - logZ);  /* P_w(y|x) */
    assert(finite(cp));
    assert(cp <= 1.0+FLT_EPSILON);

    if (s->parse[i].Pyx > 0) {
      cp -= s->parse[i].Pyx * exp(score[i] - logZc);
      assert(finite(cp));
      if (cp < -(1.0+FLT_EPSILON)) 
	fprintf(stderr, "\n\n## cp = %f, score[%d] = %g, logZc = %g, s->parse[%d].Pyx = %g, s->parse[i].Pyx * exp(score[i] - logZc) = %g\n\n",
		cp, i, score[i], logZc, i, s->parse[i].Pyx, s->parse[i].Pyx * exp(score[i] - logZc));
      assert(cp >= -(1.0+FLT_EPSILON));
    }
    
    cp *= s->Px;

    /* calculate expectations */

    for (j = 0; j < s->parse[i].nf; ++j)  /* features with 1 count */
      dL_dw[s->parse[i].f[j]] += cp;

    for (j = 0; j < s->parse[i].nfc; ++j) /* features with arbitrary counts */
      dL_dw[s->parse[i].fc[j].f] += cp * s->parse[i].fc[j].c;
  }
  return - s->Px * (logZc - logZ);
}  /* emll_sentence_stats() */
  

/*! emll_corpus_stats() returns the EM-like log loss - E_P~(x)[log E_w[P~|x]],
 *!  sets dL_dw[f] to its derivative, sets the precision/recall scores,
 *!  and increments ncorrect.
 */

Float emll_corpus_stats(corpus_type *c, const Float w[], Float dL_dw[], 
			Float *sum_g, Float *sum_p, Float *sum_w) 
{
  Float neglogP = 0;
  int i;

  *sum_g = *sum_p = *sum_w = 0;          /* zero precision/recall counters */
  for (i = 0; i < c->nfeatures; ++i)     /* zero dL_dw[] */
    dL_dw[i] = 0;

#ifdef _OPENMP
# pragma omp parallel default(shared)
  {
    int j;
    /* Float score[c->maxnparses]; 
       Float local_dL_dw[c->nfeatures]; */ /* works, but stack needs to be very large */
    Float *local_dL_dw = MALLOC(c->nfeatures*sizeof(Float));
    Float *score = MALLOC(c->maxnparses*sizeof(Float));
    Float local_sum_g = 0, local_sum_p = 0, local_sum_w = 0, local_neglogP = 0;

    for (j = 0; j < c->nfeatures; ++j)
      local_dL_dw[j] = 0;
    
# pragma omp for
    for (j = 0; j < c->nsentences; ++j)    /* collect stats from sentences */
      local_neglogP += emll_sentence_stats(&c->sentence[j], w, score, local_dL_dw, 
					   &local_sum_g, &local_sum_p, &local_sum_w);

    FREE(score);

# pragma omp critical (lmdata_emll_corpus_stats)
    {
      neglogP += local_neglogP;
      *sum_g += local_sum_g;
      *sum_p += local_sum_p;
      *sum_w += local_sum_w;
      for (j = 0; j < c->nfeatures; ++j)
	dL_dw[j] += local_dL_dw[j];
    }
    
    FREE(local_dL_dw);
  }  /* pragma omp parallel */
#else
  {
    /* Float score[c->maxnparses]; */
    Float *score = MALLOC(c->maxnparses*sizeof(Float));

    for (i = 0; i < c->nsentences; ++i)    /* collect stats from sentences */
      neglogP += emll_sentence_stats(&c->sentence[i], w, score, dL_dw, 
				     sum_g, sum_p, sum_w);

    FREE(score);
  }
#endif
  return neglogP;
}  /* emll_corpus_stats() */

/*! emll_corpus_stats_noomp() returns the EM-like log loss - E_P~(x)[log E_w[P~|x]],
 *!  sets dL_dw[f] to its derivative, sets the precision/recall scores,
 *!  and increments ncorrect.  It is just like emll_corpus_stats() except that
 *! it does not call OpenMP.
 */

Float emll_corpus_stats_noomp(corpus_type *c, const Float w[], Float dL_dw[], 
			      Float *sum_g, Float *sum_p, Float *sum_w) 
{
  Float neglogP = 0;
  int i;

  *sum_g = *sum_p = *sum_w = 0;          /* zero precision/recall counters */
  for (i = 0; i < c->nfeatures; ++i)     /* zero dL_dw[] */
    dL_dw[i] = 0;

  {
    Float *score = MALLOC(c->maxnparses*sizeof(Float));
    for (i = 0; i < c->nsentences; ++i)    /* collect stats from sentences */
      neglogP += emll_sentence_stats(&c->sentence[i], w, score, dL_dw, 
				     sum_g, sum_p, sum_w);
    FREE(score);
  }
  return neglogP;
}  /* emll_corpus_stats_noomp() */


/***********************************************************************
 *                                                                     *
 *                          pairwise log loss                          *
 *                                                                     *
 ***********************************************************************/


/*! pwlog_sentence_stats() returns the sum of the pairwise losses for
 *!  this sentence, increments dL_dw[f] with the derivative of the loss,
 *!  and increments the precision/recall scores.
 */

Float pwlog_sentence_stats(sentence_type *s, const Float w[], 
			   Float score[], Float dL_dw[],
			   Float *sum_g, Float *sum_p, Float *sum_w) 
{
  Float L = 0, best_correct_score, best_score, sum_Pyc = 0;
  int i, j, best_correct_i, best_i;

  *sum_g += s->g;

  if (s->nparses <= 0) 
    return 0;

  sentence_scores(s, w, score, &best_correct_score, &best_correct_i, 
		  &best_score, &best_i);
  *sum_p += s->parse[best_i].p;
  *sum_w += s->parse[best_i].w;
  
  if (s->Px <= 0)  /* skip statistics calculation if Px == 0 */
    return 0;

  assert(best_correct_score <= best_score);
  assert(best_correct_i >= 0);
  assert(best_correct_i < s->nparses);

  for (i = 0; i < s->nparses; ++i) 
    if (i != best_correct_i) {
      Float max_score = 
	(score[i] > best_correct_score) ? score[i] : best_correct_score;
      Float logZ = 
	log(exp(best_correct_score-max_score) + exp(score[i]-max_score))
	+ max_score;
      assert(finite(logZ));
      L -= s->Px * (best_correct_score - logZ);
      /* Pyc is conditional probability of correct parse */
      Float Pyc = exp(best_correct_score - logZ); 
      assert(Pyc >= 0);
      assert(Pyc <= 1);
      sum_Pyc += Pyc;
      /* Pyi is conditional probability of incorrect parse */
      Float Pyi = exp(score[i] - logZ);
      assert(Pyi >= 0);
      assert(Pyi <= 1);
      /* Ei is expect number of times incorrect parse occurs */
      Float Ei = s->Px * Pyi;  
      if (Ei == 0) 
	continue;
      /* calculate contribution of incorrect parse to feature expectations */
      for (j = 0; j < s->parse[i].nf; ++j)  /* features with 1 count */
	dL_dw[s->parse[i].f[j]] += Ei;
      for (j = 0; j < s->parse[i].nfc; ++j) /* features with arbitrary count */
	dL_dw[s->parse[i].fc[j].f] += Ei * s->parse[i].fc[j].c;
    }

  /* calculate contribution of correct parse to feature expectations */

  assert(sum_Pyc >= 0);
  assert(sum_Pyc <= s->nparses-1);

  /* Ec_C is difference between expected and actual number of times
     the correct parse occurs.  */
  Float Ec_C = s->Px * (sum_Pyc - (s->nparses-1));
  /* features with 1 count */
  for (j = 0; j < s->parse[best_correct_i].nf; ++j) 
    dL_dw[s->parse[best_correct_i].f[j]] += Ec_C;
  /* features with arbitrary counts */
  for (j = 0; j < s->parse[best_correct_i].nfc; ++j)
    dL_dw[s->parse[best_correct_i].fc[j].f] 
      += s->parse[best_correct_i].fc[j].c * Ec_C;
  return L;
}  /* pwlog_sentence_stats() */


Float pwlog_corpus_stats(corpus_type *c, const Float w[], Float dL_dw[], 
			Float *sum_g, Float *sum_p, Float *sum_w)
{
  Float L = 0;  /* loss */
  Float *score = MALLOC(c->maxnparses*sizeof(Float));
  int i;

  assert(score != NULL);

  *sum_g = *sum_p = *sum_w = 0;          /* zero precision/recall counters */
  for (i = 0; i < c->nfeatures; ++i)     /* zero dL_dw[] */
    dL_dw[i] = 0;
  for (i = 0; i < c->nsentences; ++i)    /* collect stats from sentences */
    L += pwlog_sentence_stats(&c->sentence[i], w, score, dL_dw, 
			      sum_g, sum_p, sum_w);
  FREE(score);
  return L;
}  /* pwlog_corpus_stats() */


/***********************************************************************
 *                                                                     *
 *                       pairwise log exp loss                         *
 *                                                                     *
 ***********************************************************************/

/*! margins() returns the minimum margin for all correct/incorrect parse
 *! pairs in the corpus, and sets margin[i] to the margin for each 
 *! correct/incorrect parse pair.
 */

Float margins(corpus_type *c, const Float w[], Float m[],
	      Float *sum_g, Float *sum_p, Float *sum_w) 
{
  Float min_margin = FLOAT_MAX;
  size_type i, j, im = 0;
  *sum_g = *sum_p = *sum_w = 0;          /* zero precision/recall counters */
  
  for (i = 0; i < c->nsentences; ++i) {
    sentence_type *s = &c->sentence[i];
    *sum_g += s->g;

    if (s->Px > 0) {
      Float correct_score = parse_score(&s->parse[s->correct_index], w);
      Float best_score = correct_score;
      size_type best_index = s->correct_index;

      for (j = 0; j < s->nparses; ++j) 
	if (j != s->correct_index) {
	  Float score = parse_score(&s->parse[j], w);
	  Float margin = correct_score - score;
	  if (score >= best_score) {
	    best_index = j;
	    best_score = score;
	  }
	  if (margin < min_margin)
	    min_margin = margin;
	  assert(im < c->nloserparses);
	  m[im++] = margin;
	}
      *sum_p += s->parse[best_index].p;
      *sum_w += s->parse[best_index].w;
    }
  }
  assert(im == c->nloserparses);
  return min_margin;
}  // margins()


/*! log_exp_corpus_stats() returns the log of the exp loss for the corpus
 *! c, sets dL_dw[j] to the derivative of the log exp loss for feature
 *! weight w[j], and sets the precision/recall statistics sum_g, sum_p
 *! and sum_w.
 */

Float log_exp_corpus_stats(corpus_type *c, const Float w[], Float dL_dw[], 
			   Float *sum_g, Float *sum_p, Float *sum_w)
{
  size_type n = c->nloserparses;
  Float *m = MALLOC(n*sizeof(Float));
  Float L, Lm = 0, min_m;
  int i, j, k, mi;

  assert(m != NULL);

  for (k = 0; k < c->nfeatures; ++k)     /* zero dL_dw[] */
    dL_dw[k] = 0;

  min_m = margins(c, w, m, sum_g, sum_p, sum_w);

  for (mi = 0; mi < n; ++mi)
    Lm += exp(min_m - m[mi]);

  L = log(Lm) - min_m;

  /* compute dL/dw */

  mi = 0;
  for (i = 0; i < c->nsentences; ++i) {
    sentence_type *s = &c->sentence[i];
    if (s->Px > 0) {
      Float c_sum = 0;
      for (j = 0; j < s->nparses; ++j) 
	if (j != s->correct_index) {
	  Float c = exp(min_m - m[mi++])/Lm;
	  c_sum += c;
	  for (k = 0; k < s->parse[j].nf; ++k)   /* 1 count features */
	    dL_dw[s->parse[j].f[k]] += c;
	  for (k = 0; k < s->parse[j].nfc; ++k)  /* arbitrary count features */
	    dL_dw[s->parse[j].fc[k].f] += c * s->parse[j].fc[k].c;
	}
      for (k = 0; k < s->parse[s->correct_index].nf; ++k)
	dL_dw[s->parse[s->correct_index].f[k]] -= c_sum;
      for (k = 0; k < s->parse[s->correct_index].nfc; ++k)
	dL_dw[s->parse[s->correct_index].fc[k].f] 
	  -= c_sum * s->parse[s->correct_index].fc[k].c;
    }
  }

  assert(mi == n);

  FREE(m);
  return L;
}  /* log_exp_corpus_stats() */


/***********************************************************************
 *                                                                     *
 *                          pairwise exp loss                          *
 *                                                                     *
 ***********************************************************************/

/*! exp_corpus_stats() returns the exp loss for the corpus
 *! c, sets dL_dw[j] to the derivative of the log exp loss for feature
 *! weight w[j], and sets the precision/recall statistics sum_g, sum_p
 *! and sum_w.
 */

Float exp_corpus_stats(corpus_type *c, const Float w[], Float dL_dw[], 
		       Float *sum_g, Float *sum_p, Float *sum_w)
{
  Float L = 0;
  int i, j, k, im = 0;
  const Float margin_cutoff = -log(FLOAT_MAX/2)/2;

  *sum_g = *sum_p = *sum_w = 0;          /* zero precision/recall counters */

  for (k = 0; k < c->nfeatures; ++k)     /* zero dL_dw[] */
    dL_dw[k] = 0;

  for (i = 0; i < c->nsentences; ++i) {
    sentence_type *s = &c->sentence[i];
    *sum_g += s->g;

    if (s->Px > 0) {
      Float correct_score = parse_score(&s->parse[s->correct_index], w);
      Float best_score = correct_score;
      size_type best_index = s->correct_index;
      Float sum_exp_nmargin = 0;
      assert(s->correct_index < s->nparses);

      for (j = 0; j < s->nparses; ++j) 
	if (j != s->correct_index) {
	  Float score = parse_score(&s->parse[j], w);
	  Float margin = correct_score - score;
	  Float exp_nmargin;

	  if (score >= best_score) {     /* save best score */
	    best_index = j;
	    best_score = score;
	  }
	  ++im;                          /* count number of incorrect parses */

	  if (margin >= margin_cutoff) {
	    exp_nmargin = exp(-margin);
	    assert(finite(exp_nmargin));
	    L += exp_nmargin;
	    assert(finite(L));
	  }
	  else {
	    exp_nmargin = exp(-margin_cutoff);
	    assert(finite(exp_nmargin));
	    L +=  (margin_cutoff+1-margin) * exp_nmargin;
	    assert(finite(L));
	  }
	  sum_exp_nmargin += exp_nmargin;
	  assert(finite(sum_exp_nmargin));
	  for (k = 0; k < s->parse[j].nf; ++k)   /* 1 count features */
	    dL_dw[s->parse[j].f[k]] += exp_nmargin;
	  for (k = 0; k < s->parse[j].nfc; ++k)  /* arbitrary count features */
	    dL_dw[s->parse[j].fc[k].f] += exp_nmargin * s->parse[j].fc[k].c;
	}
      
      for (k = 0; k < s->parse[s->correct_index].nf; ++k)
	dL_dw[s->parse[s->correct_index].f[k]] -= sum_exp_nmargin;
      for (k = 0; k < s->parse[s->correct_index].nfc; ++k)
	dL_dw[s->parse[s->correct_index].fc[k].f] 
	  -= sum_exp_nmargin * s->parse[s->correct_index].fc[k].c;

      *sum_p += s->parse[best_index].p;
      *sum_w += s->parse[best_index].w;
    }
  }
  assert(im == c->nloserparses);
  return L;
}  /* exp_corpus_stats() */


/***********************************************************************
 *                                                                     *
 *                      expected f-score optimization                  *
 *                                                                     *
 *                         fscore_corpus_stats()                       *
 *                                                                     *
 ***********************************************************************/



/*! sentence_Pyx() loads Py_x[i] with P(y[i]|x) for all parses in s.
 *! Returns the index of the last highest scoring parse.
 */

#ifndef __clang__
__inline__
#endif
int sentence_Pyx(sentence_type *s, const Float w[], Float Py_x[]) {
  
  int i, n = s->nparses, best_i = 0;
  Float Z = 0;
  const parse_type *parse = s->parse;
  Float best_score = Py_x[0] = parse_score(&parse[0], w);
  assert(s->nparses > 0);
  
  /* load Py_x[i] with parse_score(), find best_score and best_i */

  for (i = 1; i < n; ++i) {
    Float sc = Py_x[i] = parse_score(&parse[i], w);
    assert(finite(sc));
    if (sc >= best_score) {
      best_i = i;
      best_score = sc;
    }
  }

  assert(finite(best_score));

  /* compute Z */

  for (i = 0; i < n; ++i)
    Z += exp(Py_x[i] - best_score);

  assert(finite(Z));

  /* compute Py_x[] */

  for (i = 0; i < n; ++i) {
    Float P = Py_x[i] = exp(Py_x[i] - best_score) / Z;
    assert(finite(P));
  }

  return best_i;
}  /* sentence_Pyx() */


/*! fscore_sentence() 
 *!  and increments the precision/recall scores.
 */

void fscore_sentence(sentence_type *s, const Float w[], Float Py_x[], 
		     Float *E_w, Float *E_p,
		     Float sum_EDwf[], Float sum_EDpf[],
		     Float *sum_g, Float *sum_p, Float *sum_w) {

  const parse_type *parse = s->parse;
  int i, best_i, n = s->nparses;
  Float Ew = 0, Ep = 0;

  *sum_g += s->g;

  if (s->nparses <= 0) 
    return;

  best_i = sentence_Pyx(s, w, Py_x);
  *sum_p += s->parse[best_i].p;
  *sum_w += s->parse[best_i].w;
  
  if (s->Px == 0)  /* skip statistics calculation if Px == 0 */
    return;

  for (i = 0; i < n; ++i) {
    Ew += Py_x[i] * parse[i].w;
    Ep += Py_x[i] * parse[i].p;
  }

  *E_w += Ew;
  *E_p += Ep;

  for (i = 0; i < n; ++i) {
    size_type j0;
    for (j0 = 0; j0 < parse[i].nf; ++j0) {
      feature_type j = parse[i].f[j0];
      sum_EDwf[j] += Py_x[i] * (parse[i].w - Ew);
      sum_EDpf[j] += Py_x[i] * (parse[i].p - Ep);
    }
    for (j0 = 0; j0 < parse[i].nfc; ++j0) {
      feature_type j = parse[i].fc[j0].f;
      DataFloat fj = parse[i].fc[j0].c;
      sum_EDwf[j] += Py_x[i] * fj * (parse[i].w - Ew);
      sum_EDpf[j] += Py_x[i] * fj * (parse[i].p - Ep);
    }
  }
}  /* fscore_sentence() */
  

/* fscore_corpus_stats() 
 *  and increments ncorrect.
 */

Float fscore_corpus_stats(corpus_type *c, const Float w[], Float dFdw[],
			  Float *sum_g, Float *sum_p, Float *sum_w)
{
  Float *Py_x = MALLOC(c->maxnparses*sizeof(Float));
  Float *sum_EDwf = CALLOC(c->nfeatures, sizeof(Float));
  Float *sum_EDpf = CALLOC(c->nfeatures, sizeof(Float));
  Float E_w = 0, E_p = 0, D, F;
  int i, j;

  assert(Py_x != NULL);
  assert(sum_EDwf != NULL);
  assert(sum_EDpf != NULL);

  *sum_g = *sum_p = *sum_w = 0;          /* zero precision/recall counters */
  for (i = 0; i < c->nsentences; ++i)    /* collect stats from sentences */
    fscore_sentence(&c->sentence[i], w, Py_x, 
		    &E_w, &E_p, sum_EDwf, sum_EDpf,
		    sum_g, sum_p, sum_w);

  assert(finite(E_w));
  assert(finite(E_p));

  D = E_p + *sum_g;
  assert(finite(D));

  F = 2 * E_w / D;
  assert(finite(F));

  for (j = 0; j < c->nfeatures; ++j) {
    dFdw[j] = 2*sum_EDwf[j]/D - F*sum_EDpf[j]/D;
    assert(finite(dFdw[j]));
  }
  
  FREE(Py_x);
  FREE(sum_EDwf);
  FREE(sum_EDpf);
  return F;

}  /* fscore_corpus_stats() */


/***********************************************************************
 *                                                                     *
 *                        averaged perceptron                          *
 *                                                                     *
 ***********************************************************************/

/*! ap_update1() performs an averaged perceptron update on feature j.
 */

__inline__ static
void ap_update1(feature_type j, Float w[], Float update, Float sum_w[],
		size_type it, size_type changed[]) {
  sum_w[j] += (it - changed[j]) * w[j];
  changed[j] = it;
  w[j] += update;
}  /* ap_update1() */

/*! ap_sentence_scores() loads score[] with the scores of all parses in s,
 *! and sets best_correct_score, best_correct_i, best_score and best_i.
 */

void ap_sentence_scores(sentence_type *s, const Float w[], 
			Float *best_correct_score, int *best_correct_i,
			Float *best_score, int *best_i) {
  int i;
  Float sc;
  assert(s->nparses > 0);

  *best_i = 0;
  *best_correct_i = -1;

  *best_score = sc = parse_score(&s->parse[0], w);
  if (s->parse[0].Pyx > 0) {
    *best_correct_i = 0;
    *best_correct_score = sc;
  }

  for (i = 1; i < s->nparses; ++i) {
    sc = parse_score(&s->parse[i], w);
    if (sc >= *best_score) {
      *best_i = i;
      *best_score = sc;
    }
    if (s->parse[i].Pyx > 0 && (*best_correct_i < 0 || sc >= *best_correct_score)) {
      *best_correct_i = i;
      *best_correct_score = sc;
    }
  }

  assert(finite(*best_score));
  assert(s->Px == 0 || (*best_correct_i >= 0 && *best_correct_i < s->nparses));
  /* assert(best_correct_i >= 0); assert(finite(*best_correct_score)); */
}  /* ap_sentence_scores() */


/*! ap_wd_featureweight() returns the weight on feature j, accounting for 
 *! weight decay.  It also updates the feature weight to the current time
 *! step and updates sum_w[j] appropriately.
 */

__inline__ static
Float ap_wd_featureweight(feature_type j, Float w[], Float weightdecay, 
			  Float sum_w[], size_type it, size_type changed[]) 
{
  size_type dn = it - changed[j];
  Float f = pow(1-weightdecay, dn-1);
  changed[j] = it;
  sum_w[j] += w[j] * (1 - f*(1-weightdecay))/weightdecay;
  return w[j] *= f;  /* return discounted weight */
}  /* ap_wd_featureweight() */

/*! ap_parse_wd_score() calculates the score for parse p, discounting
 *! the weight and updating the weight vector appropriately.
 */

__inline__ static
Float ap_parse_wd_score(const parse_type *p, Float w[], Float weightdecay, 
			Float sum_w[], size_type it, size_type changed[])
{
  int i;
  Float score = 0;
  /* features with count of 1 */
  for (i = 0; i < p->nf; ++i)
    score += ap_wd_featureweight(p->f[i], w, weightdecay, sum_w, it, changed);
  /* features with arbitrary count */
  for (i = 0; i < p->nfc; ++i)
    score += p->fc[i].c 
      * ap_wd_featureweight(p->fc[i].f, w, weightdecay, sum_w, it, changed);
  return score;
}  /* ap_parse_wd_score() */

/*! ap_wd_sentence_scores() loads score[] with the scores of all parses in s,
 *! and sets best_correct_score, best_correct_i, best_score and best_i.
 */

void ap_wd_sentence_scores(sentence_type *s, Float w[], Float weightdecay,
			   Float sum_w[], size_type it, size_type changed[],
			   Float *best_correct_score, int *best_correct_i,
			   Float *best_score, int *best_i) {
  int i;
  Float sc;
  assert(s->nparses > 0);

  *best_i = 0;
  *best_correct_i = -1;

  *best_score = sc 
    = ap_parse_wd_score(&s->parse[0], w, weightdecay, sum_w, it, changed);
  if (s->parse[0].Pyx > 0) {
    *best_correct_i = 0;
    *best_correct_score = sc;
  }

  for (i = 1; i < s->nparses; ++i) {
    sc = ap_parse_wd_score(&s->parse[i], w, weightdecay, sum_w, it, changed);
    if (sc >= *best_score) {
      *best_i = i;
      *best_score = sc;
    }
    if (s->parse[i].Pyx > 0 
	&& (*best_correct_i < 0 || sc >= *best_correct_score)) {
      *best_correct_i = i;
      *best_correct_score = sc;
    }
  }

  assert(finite(*best_score));
  assert(s->Px == 0 || (*best_correct_i >= 0 && *best_correct_i < s->nparses));
  /* assert(best_correct_i >= 0); assert(finite(*best_correct_score)); */
}  /* ap_wd_sentence_scores() */


/*! ap_sentence() handles a single round of the averaged perceptron.
 *!
 *!  s          - sentence data
 *!  w          - current weight vector, updated if necessary
 *!  dw         - weight update amount
 *!  weightdecay - weight decay
 *!  sum_w      - cumulative sum of weight vectors
 *!  it         - current iteration
 *!  changed[k] - iteration at which w[k] was last changed
 */

void ap_sentence(sentence_type *s, Float w[], Float dw, Float weightdecay,
		 Float sum_w[], size_type it, size_type changed[])
{
  Float best_correct_score, best_score;
  int best_i, best_correct_i;
  if (weightdecay == 0)
    ap_sentence_scores(s, w, &best_correct_score, &best_correct_i, 
		       &best_score, &best_i);
  else
    ap_wd_sentence_scores(s, w, weightdecay, sum_w, it, changed,
			  &best_correct_score, &best_correct_i, 
			  &best_score, &best_i);

  if (best_correct_score <= best_score) { 
    /* update between parse[best_correct_i] and parse[best_i] */
    size_type j;
    parse_type *correct = &s->parse[best_correct_i];
    parse_type *winner = &s->parse[best_i];

    if (winner->Pyx >= correct->Pyx)
      return;

    /* multiply update weight by importance of this pair */
    assert(correct->Pyx > 0);
    dw *= s->Px * fabs(correct->Pyx - winner->Pyx)/correct->Pyx;

    /* subtract winner's feature counts */
    for (j = 0; j < winner->nf; ++j) 
      ap_update1(winner->f[j], w, -dw, sum_w, it, changed);
    for (j = 0; j < winner->nfc; ++j)
      ap_update1(winner->fc[j].f, w, -dw*winner->fc[j].c, sum_w, it, changed);

    /* add correct's feature counts */
    for (j = 0; j < correct->nf; ++j)
      ap_update1(correct->f[j], w, dw, sum_w, it, changed);
    for (j = 0; j < correct->nfc; ++j)
      ap_update1(correct->fc[j].f, w, dw*correct->fc[j].c, sum_w, it, changed);
  }
}  /* ap_sentence() */


/*! wap_sentence() handles a single round of the weighted averaged perceptron.
 *!
 *!  s          - sentence data
 *!  w          - current weight vector, updated if necessary
 *!  dw         - weight update factor
 *!  feat_class - feature -> class vector
 *!  class_dw   - class weight update factor
 *!  sum_w      - cumulative sum of weight vectors
 *!  it         - current iteration
 *!  changed[k] - iteration at which w[k] was last changed
 */

void wap_sentence(sentence_type *s, Float w[], 
		  Float dw, const size_type feat_class[], const Float class_dw[],
		  Float sum_w[], size_type it, size_type changed[])
{
  Float best_correct_score, best_score;
  int best_correct_i = 0, best_i = 0;
  ap_sentence_scores(s, w, &best_correct_score, &best_correct_i, &best_score, &best_i);

  if (best_correct_score <= best_score) { 
    /* update between parse[best_correct_i] and parse[best_i] */
    size_type j;
    parse_type *correct = &s->parse[best_correct_i];
    parse_type *winner = &s->parse[best_i];

    if (winner->Pyx >= correct->Pyx)
      return;

    /* multiply update weight by importance of this pair */
    dw *= s->Px * fabs(correct->Pyx - winner->Pyx)/correct->Pyx;

    /* subtract winner's feature counts */
    for (j = 0; j < winner->nf; ++j) {
      size_type f = winner->f[j];
      ap_update1(f, w, -dw*class_dw[feat_class[f]], sum_w, it, changed);
    }
    for (j = 0; j < winner->nfc; ++j) {
      size_type f = winner->fc[j].f;
      ap_update1(f, w, -dw*winner->fc[j].c*class_dw[feat_class[f]], 
		 sum_w, it, changed);
    }

    /* add correct's feature counts */
    for (j = 0; j < correct->nf; ++j) {
      size_type f = correct->f[j];
      ap_update1(f, w, dw*class_dw[feat_class[f]], sum_w, it, changed);
    }
    for (j = 0; j < correct->nfc; ++j) {
      size_type f = correct->fc[j].f;
      ap_update1(f, w, dw*correct->fc[j].c*class_dw[feat_class[f]], sum_w, it, changed);
    }
  }
}  /* wap_sentence() */


/***********************************************************************
 *                                                                     *
 *                     logistic neural network                         *
 *                                                                     *
 ***********************************************************************/

/*! lnn_unpack_weights_type() sets a lnn_weights_type to point to the appropriate
 *! places in the single long weight vector w[].
 */

void lnn_unpack_weights_type(Float w[], size_type nhidden, size_type nfeatures,
			     lnn_weights_type *wt) 
{
  wt->w1 = w;
  wt->b0 = w+nhidden;
  wt->w0 = w+2*nhidden;
}  /* lnn_unpack_weights_type() */


/*! lnn_parse_score() sets score0[] (the hidden units scores) and
 *! returns score1 for parse *p. 
 */

__inline__
static Float lnn_parse_score(parse_type *p, const lnn_weights_type* wt, 
			     size_type nhidden, size_type nfeatures,
			     Float score0[]) {
  Float score1 = 0;
  int j;
  for (j = 0; j < nhidden; ++j) {
    Float input = parse_score(p, &wt->w0[j*nfeatures]) + wt->b0[j];
    score0[j] = tanh(input);  /* tanh sigmoid activation function */
    score1 += wt->w1[j] * score0[j];
  }
  return score1;
}  /* lnn_parse_score() */


/*! lnn_sentence_scores() loads score[] with the scores of all parses in s,
 *! and sets best_correct_score and best_score.  
 */

void lnn_sentence_scores(sentence_type *s, const lnn_weights_type* wt, 
			 size_type nhidden, size_type nfeatures,
			 Float score0[], Float score1[],
			 Float *best_correct_score, int *best_correct_i,
			 Float *best_score, int *best_i) {
  int i;
  Float sc;
  assert(s->nparses > 0);

  *best_i = 0;
  *best_correct_i = -1;

  *best_score = score1[0] = sc 
    = lnn_parse_score(&s->parse[0], wt, nhidden, nfeatures, &score0[0]);

  if (s->parse[0].Pyx > 0) {
    *best_correct_i = 0;
    *best_correct_score = sc;
  }

  for (i = 1; i < s->nparses; ++i) {
    score1[i] = sc 
      = lnn_parse_score(&s->parse[i], wt, nhidden, nfeatures, &score0[i*nhidden]);
    if (sc >= *best_score) {
      *best_i = i;
      *best_score = sc;
    }
    if (s->parse[i].Pyx > 0 && (*best_correct_i < 0 || sc > *best_correct_score)) {
      *best_correct_i = i;
      *best_correct_score = sc;
    }
  }

  assert(finite(*best_score));
  assert(s->Px == 0 || (*best_correct_i >= 0 && *best_correct_i < s->nparses));
  /* assert(best_correct_i >= 0); assert(finite(*best_correct_score)); */
}  /* lnn_sentence_scores() */


/*! lnn_sentence_stats() returns - log P~(x) (E_P~[w.f|x] - log Z_w(x))
 *!  increments E_Ew[f] with the difference of the expected 
 *!  value of f for the winners and for all parses,
 *!  and increments the precision/recall scores.
 */

Float lnn_sentence_stats(sentence_type *s, const lnn_weights_type* wt,
			 size_type nhidden, size_type nfeatures,
			 Float score0[], Float score1[], 
			 const lnn_weights_type* dL_dwt,
			 Float *sum_g, Float *sum_p, Float *sum_w) 
{ 
  Float best_correct_score, best_score;
  int i, j, k, best_i = 0, best_correct_i = 0;
  Float Z = 0, logZ, Ecorrect_score = 0;

  *sum_g += s->g;

  lnn_sentence_scores(s, wt, nhidden, nfeatures,
		      score0, score1, 
		      &best_correct_score, &best_correct_i,
		      &best_score, &best_i);

  *sum_p += s->parse[best_i].p;
  *sum_w += s->parse[best_i].w;
  
  if (s->Px == 0)  /* skip statistics calculation if Px == 0 */
    return 0;

  assert(best_correct_score <= best_score);

  for (i = 0; i < s->nparses; ++i) {   /* compute Z and Zw */
    assert(score1[i] <= best_score);
    Z += exp(score1[i] - best_score);
    assert(finite(Z));
    if (s->parse[i].Pyx > 0) 
      Ecorrect_score += s->parse[i].Pyx * score1[i];
  }

  logZ = log(Z) + best_score;

  /* calculate expectations */

  for (i = 0; i < s->nparses; ++i) {
    Float cp = exp(score1[i] - logZ);  /* P_w(y|x) */
    assert(finite(cp));

    if (s->parse[i].Pyx > 0)  /* P_e(y|x)  */
      cp -= s->parse[i].Pyx;

    assert(cp >= -1.0);
    assert(cp <= 1.0);
    
    cp *= s->Px;   /* multiply in weight of sentence */

    /* calculate derivatives */

    for (j = 0; j < nhidden; ++j) {
      Float g = score0[i*nhidden+j];
      Float dg_dx = 1 - g*g;  /* for tanh activation function */
      Float backward = cp * wt->w1[j] * dg_dx;
      dL_dwt->w1[j] += cp * score0[i*nhidden+j];
      dL_dwt->b0[j] += backward;
      
      for (k = 0; k < s->parse[i].nf; ++k)  /* features with 1 count */
	dL_dwt->w0[j*nfeatures+s->parse[i].f[k]] += backward;
      
      for (k = 0; k < s->parse[i].nfc; ++k) /* features with arbitrary counts */
	dL_dwt->w0[j*nfeatures+s->parse[i].fc[k].f] 
	  += backward * s->parse[i].fc[k].c;
    }
  }
  return - s->Px * (Ecorrect_score - logZ);
}  /* lnn_sentence_stats() */
  

/*! lnn_corpus_stats() returns - log P(winners|parses), 
 *!  sets dL_dw[], and increments sum_g, sum_p and sum_w.
 */
  
Float lnn_corpus_stats(corpus_type *c, size_type nhidden, 
		       Float w[], Float dL_dw[],
		       Float *sum_g, Float *sum_p, Float *sum_w) 
{
  lnn_weights_type wt, dL_dwt;
  Float neglogP = 0;
  Float *score1 = MALLOC(c->maxnparses*sizeof(Float));
  Float *score0 = MALLOC(nhidden * c->maxnparses*sizeof(Float));

  int i;
  
  assert(score1 != NULL);
  assert(score0 != NULL);

  *sum_g = *sum_p = *sum_w = 0;          /* zero precision/recall counters */

  for (i = 0; i < nhidden*(c->nfeatures+2); ++i) 
    dL_dw[i] = 0;    /* zero dL_dw[] */

  lnn_unpack_weights_type(w, nhidden, c->nfeatures, &wt);
  lnn_unpack_weights_type(dL_dw, nhidden, c->nfeatures, &dL_dwt);

  for (i = 0; i < c->nsentences; ++i)  /* collect stats from sentences */
    neglogP += lnn_sentence_stats(&c->sentence[i], &wt, nhidden, c->nfeatures, 
				  score0, score1, &dL_dwt, sum_g, sum_p, sum_w);

  FREE(score0);
  FREE(score1);
  return neglogP;
}  /* lnn_corpus_stats() */
