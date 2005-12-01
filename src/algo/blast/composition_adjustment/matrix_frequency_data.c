/* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================*/

/** @file joint_probs.c
 *
 * @author Yi-Kuo Yu, Alejandro Schaffer, E. Michael Gertz
 *
 * Joint probabilities for specific matrices.
 */
#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] =
    "$Id$";
#endif /* SKIP_DOXYGEN_PROCESSING */

#include <stdlib.h>
#include <algo/blast/core/ncbi_std.h>
#include <algo/blast/composition_adjustment/composition_constants.h>
#include <algo/blast/composition_adjustment/matrix_frequency_data.h>

/* bound on error for sum of probabilities*/
static const double kProbSumTolerance = 0.000000001;

/* Joint probabilities for BLOSUM62 */
static double
BLOSUM62_JOINT_PROBS[COMPO_NUM_TRUE_AA][COMPO_NUM_TRUE_AA]
= {
    {0.021516461557, 0.002341028532, 0.001941062549, 0.002160193055,
     0.001595828537, 0.001934059173, 0.002990874959, 0.005831307116,
     0.001108651421, 0.003181451207, 0.004450432543, 0.003350994862,
     0.001330482798, 0.001634084433, 0.002159278003, 0.006261897426,
     0.003735752688, 0.000404784037, 0.001298558985, 0.005124343367},
    {0.002341028532, 0.017737158563, 0.001969132731, 0.001581985934,
     0.000393496788, 0.002483620870, 0.002678135197, 0.001721914295,
     0.001230766890, 0.001239704106, 0.002418976127, 0.006214150782,
     0.000796884039, 0.000932356719, 0.000959872904, 0.002260870847,
     0.001779897849, 0.000265310579, 0.000918577576, 0.001588408095},
    {0.001941062549, 0.001969132731, 0.014105369019, 0.003711182199,
     0.000436559586, 0.001528401416, 0.002205231268, 0.002856026580,
     0.001423459827, 0.000986015608, 0.001369776043, 0.002436729322,
     0.000521972796, 0.000746722150, 0.000858953243, 0.003131380307,
     0.002237168191, 0.000161021675, 0.000695990541, 0.001203509685},
    {0.002160193055, 0.001581985934, 0.003711182199, 0.021213070328,
     0.000397349231, 0.001642988683, 0.004909362115, 0.002510933422,
     0.000948355160, 0.001226071189, 0.001524412852, 0.002443951825,
     0.000458902921, 0.000759393269, 0.001235481304, 0.002791458183,
     0.001886707235, 0.000161498946, 0.000595157039, 0.001320931409},
    {0.001595828537, 0.000393496788, 0.000436559586, 0.000397349231,
     0.011902428201, 0.000309689150, 0.000380965445, 0.000768969543,
     0.000229437747, 0.001092222651, 0.001570843250, 0.000500631539,
     0.000373569136, 0.000512643056, 0.000360439075, 0.001038049531,
     0.000932287369, 0.000144869300, 0.000344932387, 0.001370634611},
    {0.001934059173, 0.002483620870, 0.001528401416, 0.001642988683,
     0.000309689150, 0.007348611171, 0.003545322222, 0.001374101100,
     0.001045402587, 0.000891574240, 0.001623152279, 0.003116305001,
     0.000735592074, 0.000544610751, 0.000849940593, 0.001893917959,
     0.001381521088, 0.000228499204, 0.000674510708, 0.001174481769},
    {0.002990874959, 0.002678135197, 0.002205231268, 0.004909362115,
     0.000380965445, 0.003545322222, 0.016058942448, 0.001941788215,
     0.001359354087, 0.001208575016, 0.002010620195, 0.004137352463,
     0.000671608129, 0.000848058651, 0.001418534945, 0.002949177015,
     0.002049363253, 0.000264084965, 0.000864998825, 0.001706373779},
    {0.005831307116, 0.001721914295, 0.002856026580, 0.002510933422,
     0.000768969543, 0.001374101100, 0.001941788215, 0.037833882792,
     0.000956438296, 0.001381594180, 0.002100349645, 0.002551728599,
     0.000726329019, 0.001201930393, 0.001363538639, 0.003819521365,
     0.002185818204, 0.000406753457, 0.000831463001, 0.001832653843},
    {0.001108651421, 0.001230766890, 0.001423459827, 0.000948355160,
     0.000229437747, 0.001045402587, 0.001359354087, 0.000956438296,
     0.009268821027, 0.000575006579, 0.000990341860, 0.001186603601,
     0.000377383962, 0.000807129053, 0.000477177871, 0.001100800912,
     0.000744015818, 0.000151511190, 0.001515361861, 0.000650302833},
    {0.003181451207, 0.001239704106, 0.000986015608, 0.001226071189,
     0.001092222651, 0.000891574240, 0.001208575016, 0.001381594180,
     0.000575006579, 0.018297094930, 0.011372374833, 0.001566332194,
     0.002471405322, 0.003035353009, 0.001002322534, 0.001716150165,
     0.002683992649, 0.000360556333, 0.001366091300, 0.011965802769},
    {0.004450432543, 0.002418976127, 0.001369776043, 0.001524412852,
     0.001570843250, 0.001623152279, 0.002010620195, 0.002100349645,
     0.000990341860, 0.011372374833, 0.037325284430, 0.002482344486,
     0.004923694031, 0.005449900864, 0.001421696216, 0.002434190706,
     0.003337092433, 0.000733421681, 0.002210504676, 0.009545821406},
    {0.003350994862, 0.006214150782, 0.002436729322, 0.002443951825,
     0.000500631539, 0.003116305001, 0.004137352463, 0.002551728599,
     0.001186603601, 0.001566332194, 0.002482344486, 0.016147683460,
     0.000901118905, 0.000950170174, 0.001578353818, 0.003104386139,
     0.002360691115, 0.000272260749, 0.000996404634, 0.001952015271},
    {0.001330482798, 0.000796884039, 0.000521972796, 0.000458902921,
     0.000373569136, 0.000735592074, 0.000671608129, 0.000726329019,
     0.000377383962, 0.002471405322, 0.004923694031, 0.000901118905,
     0.003994917914, 0.001184353682, 0.000404888644, 0.000847632455,
     0.001004584462, 0.000197602804, 0.000563431813, 0.002301832938},
    {0.001634084433, 0.000932356719, 0.000746722150, 0.000759393269,
     0.000512643056, 0.000544610751, 0.000848058651, 0.001201930393,
     0.000807129053, 0.003035353009, 0.005449900864, 0.000950170174,
     0.001184353682, 0.018273718971, 0.000525642239, 0.001195904180,
     0.001167245623, 0.000851298193, 0.004226922511, 0.002601386501},
    {0.002159278003, 0.000959872904, 0.000858953243, 0.001235481304,
     0.000360439075, 0.000849940593, 0.001418534945, 0.001363538639,
     0.000477177871, 0.001002322534, 0.001421696216, 0.001578353818,
     0.000404888644, 0.000525642239, 0.019101516083, 0.001670397698,
     0.001352022511, 0.000141505490, 0.000450817134, 0.001257818591},
    {0.006261897426, 0.002260870847, 0.003131380307, 0.002791458183,
     0.001038049531, 0.001893917959, 0.002949177015, 0.003819521365,
     0.001100800912, 0.001716150165, 0.002434190706, 0.003104386139,
     0.000847632455, 0.001195904180, 0.001670397698, 0.012524165008,
     0.004695393160, 0.000286147117, 0.001025667373, 0.002373134246},
    {0.003735752688, 0.001779897849, 0.002237168191, 0.001886707235,
     0.000932287369, 0.001381521088, 0.002049363253, 0.002185818204,
     0.000744015818, 0.002683992649, 0.003337092433, 0.002360691115,
     0.001004584462, 0.001167245623, 0.001352022511, 0.004695393160,
     0.012524453183, 0.000287144142, 0.000940528155, 0.003660378402},
    {0.000404784037, 0.000265310579, 0.000161021675, 0.000161498946,
     0.000144869300, 0.000228499204, 0.000264084965, 0.000406753457,
     0.000151511190, 0.000360556333, 0.000733421681, 0.000272260749,
     0.000197602804, 0.000851298193, 0.000141505490, 0.000286147117,
     0.000287144142, 0.006479671265, 0.000886553355, 0.000357440337},
    {0.001298558985, 0.000918577576, 0.000695990541, 0.000595157039,
     0.000344932387, 0.000674510708, 0.000864998825, 0.000831463001,
     0.001515361861, 0.001366091300, 0.002210504676, 0.000996404634,
     0.000563431813, 0.004226922511, 0.000450817134, 0.001025667373,
     0.000940528155, 0.000886553355, 0.010185916203, 0.001555728244},
    {0.005124343367, 0.001588408095, 0.001203509685, 0.001320931409,
     0.001370634611, 0.001174481769, 0.001706373779, 0.001832653843,
     0.000650302833, 0.011965802769, 0.009545821406, 0.001952015271,
     0.002301832938, 0.002601386501, 0.001257818591, 0.002373134246,
     0.003660378402, 0.000357440337, 0.001555728244, 0.019815247974}
};



/* Background frequencies for BLOSUM62 */
static double BLOSUM62_bg[COMPO_NUM_TRUE_AA] =
    { 0.0742356686, 0.0515874541, 0.0446395713, 0.0536092024, 0.0246865086,
      0.0342500470, 0.0543174458, 0.0741431988, 0.0262119099, 0.0679331197,
      0.0989057232, 0.0581774322, 0.0249972837, 0.0473970070, 0.0385382904,
      0.0572279733, 0.0508996546, 0.0130298868, 0.0322925130, 0.0729201182
    };


int Blast_FrequencyDataIsAvailable(const char *matrix_name)
{
    return NULL != Blast_GetMatrixBackgroundFreq(matrix_name);
}


/** Retrieve the background letter probabilities implicitly used in
 * constructing the score matrix matrix_name. */
const double *
Blast_GetMatrixBackgroundFreq(const char *matrix_name)
{
    if (0 == strcmp(matrix_name, "BLOSUM62")) {
        return BLOSUM62_bg;
    } else {                    /* default */
        fprintf(stderr, "matrix not supported, exit now! \n");
        return NULL;
    }
}


/**
 * Get joint probabilities for the named matrix.
 *
 * @param probs        the joint probabilities [out]
 * @param row_sums     sum of the values in each row of probs [out]
 * @param col_sums     sum of the values in each column of probs [out]
 * @param matrixName   the name of the matrix sought [in]
 * @returns 0 if successful; -1 if the named matrix is not known.
 */
int
Blast_GetJointProbsForMatrix(double ** probs, double row_sums[],
                             double col_sums[], const char *matrixName)
{
    double sum;            /* sum of all joint probabilities -- should
                              be close to one */
    int i, j;              /* loop indices */
    /* The joint probabilities of the selected matrix */
    double (*joint_probs)[COMPO_NUM_TRUE_AA];

    /* Choose the matrix */
    if (0 == strcmp("BLOSUM62", matrixName)) {
        joint_probs = BLOSUM62_JOINT_PROBS;
    } else {
        fprintf(stderr, "matrix %s is not supported "
                "for RE based adjustment\n", matrixName);
        return -1;
    }
    sum = 0.0;
    for (i = 0;  i < COMPO_NUM_TRUE_AA;  i++) {
        for (j = 0;  j < COMPO_NUM_TRUE_AA;  j++) {
            sum += joint_probs[i][j];
        }
    }
    assert(fabs(sum - 1.0) < kProbSumTolerance);
    /* Normalize and record the data */
    for (j = 0;  j < COMPO_NUM_TRUE_AA;  j++) {
        col_sums[j] = 0.0;
    }
    for (i = 0;  i < COMPO_NUM_TRUE_AA;  i++) {
        row_sums[i] = 0.0;
        for (j = 0;  j < COMPO_NUM_TRUE_AA;  j++) {
            double probij = joint_probs[i][j];

            probs[i][j]  = probij/sum;
            row_sums[i] += probij/sum;
            col_sums[j] += probij/sum;
        }
    }
    return 0;
}
