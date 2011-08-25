/*  $Id$
 * ===========================================================================
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
 * ===========================================================================
 *
 * Authors:  Frank Ludwig
 *
 * File Description:  Write gff file
 *
 */

#include <ncbi_pch.hpp>

#include <objects/seqset/Seq_entry.hpp>
#include <objects/seq/Seq_annot.hpp>
#include <objects/seq/Annot_descr.hpp>
#include <objects/seqfeat/Seq_feat.hpp>

#include <objects/general/Object_id.hpp>
#include <objects/general/User_object.hpp>
#include <objects/general/User_field.hpp>
#include <objects/general/Dbtag.hpp>
#include <objects/seqfeat/Feat_id.hpp>
#include <objects/seqfeat/Gb_qual.hpp>
#include <objects/seqfeat/Cdregion.hpp>
#include <objects/seqfeat/SeqFeatXref.hpp>
#include <objects/seqalign/Dense_seg.hpp>
#include <objects/seqalign/Spliced_seg.hpp>
#include <objects/seqalign/Seq_align_set.hpp>
#include <objects/seqalign/Score.hpp>

#include <objmgr/feat_ci.hpp>
#include <objmgr/seqdesc_ci.hpp>
#include <objmgr/mapped_feat.hpp>
#include <objmgr/util/feature.hpp>
#include <objmgr/util/sequence.hpp>

#include <objtools/writers/gff3_write_data.hpp>
#include <objtools/writers/gff3_alignment_data.hpp>
#include <objtools/writers/gff3_writer.hpp>
#include <objtools/alnmgr/alnmap.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);

#define INSERTION(sf, tf) ( ((sf) &  CAlnMap::fSeq) && !((tf) &  CAlnMap::fSeq) )
#define DELETION(sf, tf) ( !((sf) &  CAlnMap::fSeq) && ((tf) &  CAlnMap::fSeq) )
#define MATCH(sf, tf) ( ((sf) &  CAlnMap::fSeq) && ((tf) &  CAlnMap::fSeq) )

//  ----------------------------------------------------------------------------
static CConstRef<CSeq_id> s_GetSourceId(
    const CSeq_id& id, CScope& scope )
//  ----------------------------------------------------------------------------
{
    try {
        return sequence::GetId(id, scope, sequence::eGetId_ForceAcc).GetSeqId();
    }
    catch (CException&) {
    }
    return CConstRef<CSeq_id>(&id);
}

//  ----------------------------------------------------------------------------
CGff3Writer::CGff3Writer(
    CScope& scope, 
    CNcbiOstream& ostr,
    unsigned int uFlags ) :
//  ----------------------------------------------------------------------------
    CGff2Writer( scope, ostr, uFlags )
{
    m_uRecordId = 1;
    m_uPendingGeneId = 0;
    m_uPendingMrnaId = 0;
    m_uPendingTrnaId = 0;
    m_uPendingExonId = 0;
    m_uPendingCdsId = 0;
    m_uPendingGenericId = 0;
};

//  ----------------------------------------------------------------------------
CGff3Writer::CGff3Writer(
    CNcbiOstream& ostr,
    unsigned int uFlags ) :
//  ----------------------------------------------------------------------------
    CGff2Writer( ostr, uFlags )
{
    m_uRecordId = 1;
    m_uPendingGeneId = 0;
    m_uPendingMrnaId = 0;
    m_uPendingExonId = 0;
    m_uPendingCdsId = 0;
    m_uPendingTrnaId = 0;
    m_uPendingGenericId = 0;
};

//  ----------------------------------------------------------------------------
CGff3Writer::~CGff3Writer()
//  ----------------------------------------------------------------------------
{
};

//  ----------------------------------------------------------------------------
bool CGff3Writer::x_WriteAlign( 
    const CSeq_align& align,
    bool bInvertWidth )
//  ----------------------------------------------------------------------------
{
    if ( ! align.IsSetSegs() ) {
        cerr << "Object type not supported." << endl;
        return true;
    }

    switch( align.GetSegs().Which() ) {
    default:
        break;
    case CSeq_align::TSegs::e_Denseg:
        return x_WriteAlignDenseg( align, bInvertWidth );
    case CSeq_align::TSegs::e_Spliced:
        return x_WriteAlignSpliced( align, bInvertWidth );
    case CSeq_align::TSegs::e_Disc:
        return x_WriteAlignDisc( align, bInvertWidth );
    }
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Writer::x_WriteAlignDisc( 
    const CSeq_align& align,
    bool bInvertWidth )
//  ----------------------------------------------------------------------------
{
    typedef CSeq_align_set::Tdata::const_iterator CASCIT;

    const CSeq_align_set::Tdata& data = align.GetSegs().GetDisc().Get();
    for ( CASCIT cit = data.begin(); cit != data.end(); ++cit ) {
        x_WriteAlign( **cit, bInvertWidth );
    }
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Writer::x_WriteAlignSpliced( 
    const CSeq_align& align,
    bool bInvertWidth )
//  ----------------------------------------------------------------------------
{
    CRef<CSeq_align> pSa = align.GetSegs().GetSpliced().AsDiscSeg();
    if ( align.IsSetScore() ) {
        pSa->SetScore().insert( pSa->SetScore().end(),
            align.GetScore().begin(), align.GetScore().end() );
    }
    return x_WriteAlign(*pSa );
}

//  ----------------------------------------------------------------------------
bool CGff3Writer::x_WriteAlignDenseg( 
    const CSeq_align& align,
    bool bInvertWidth )
//  ----------------------------------------------------------------------------
{
    const CSeq_id& productId = align.GetSeq_id( 0 );
    CBioseq_Handle bsh = m_pScope->GetBioseqHandle( productId );
    CRef<CSeq_id> pTargetId( new CSeq_id );
    pTargetId->Assign( *sequence::GetId(
        bsh, sequence::eGetId_Best).GetSeqId() );

    const CDense_seg& ds = align.GetSegs().GetDenseg();
    CRef<CDense_seg> ds_filled = ds.FillUnaligned();
    if ( bInvertWidth ) {
//        ds_filled->ResetWidths();
    }

    CAlnMap align_map( *ds_filled );

    int iTargetRow = -1;
    for ( int row = 0;  row < align_map.GetNumRows();  ++row ) {
        if ( sequence::IsSameBioseq( 
            align_map.GetSeqId( row ), *pTargetId, m_pScope ) ) {
            iTargetRow = row;
            break;
        }
    }
    if ( iTargetRow == -1 ) {
        cerr << "No alignment row containing primary ID." << endl;
        return false;
    }

    TSeqPos iTargetWidth = 1;
    if ( static_cast<size_t>( iTargetRow ) < ds.GetWidths().size() ) {
        iTargetWidth = ds.GetWidths()[ iTargetRow ];
    }

    ENa_strand targetStrand = eNa_strand_plus;
    if ( align_map.StrandSign( iTargetRow ) != 1 ) {
        targetStrand = eNa_strand_minus;
    }

    for ( int iSourceRow = 0;  iSourceRow < align_map.GetNumRows();  ++iSourceRow ) {
        if ( iSourceRow == iTargetRow ) {
            continue;
        }
        CGffAlignmentRecord record( m_uFlags, m_uRecordId++ );

        // Obtain and report basic source information:
        CConstRef<CSeq_id> pSourceId =
            s_GetSourceId( align_map.GetSeqId(iSourceRow), *m_pScope );
        TSeqPos iSourceWidth = 1;
        if ( static_cast<size_t>( iSourceRow ) < ds.GetWidths().size() ) {
            iSourceWidth = ds.GetWidths()[ iSourceRow ];
        }
        ENa_strand sourceStrand = eNa_strand_plus;
        if ( align_map.StrandSign( iSourceRow ) != 1 ) {
            sourceStrand = eNa_strand_minus;
        }
        record.SetSourceLocation( *pSourceId, sourceStrand );

        // Place insertions, deletion, matches, compute resulting source and target 
        //  ranges:
        for ( int i0 = 0;  i0 < align_map.GetNumSegs();  ++i0 ) {

            CAlnMap::TSignedRange targetPiece = align_map.GetRange( iTargetRow, i0);
            CAlnMap::TSignedRange sourcePiece = align_map.GetRange( iSourceRow, i0 );
            CAlnMap::TSegTypeFlags targetFlags = align_map.GetSegType( iTargetRow, i0 );
            CAlnMap::TSegTypeFlags sourceFlags = align_map.GetSegType( iSourceRow, i0 );

            if ( INSERTION( targetFlags, sourceFlags ) ) {
                record.AddInsertion( CAlnMap::TSignedRange( 
                    targetPiece.GetFrom() / iTargetWidth, 
                    targetPiece.GetTo() / iTargetWidth ) );
            }
            if ( DELETION( targetFlags, sourceFlags ) ) {
                record.AddDeletion( CAlnMap::TSignedRange( 
                    sourcePiece.GetFrom() / iSourceWidth, 
                    sourcePiece.GetTo() / iSourceWidth ) );
            }
            if ( MATCH( targetFlags, sourceFlags ) ) {
                record.AddMatch( 
                    CAlnMap::TSignedRange( 
                        sourcePiece.GetFrom() / iSourceWidth, 
                        sourcePiece.GetTo() / iSourceWidth ), 
                    CAlnMap::TSignedRange( 
                        targetPiece.GetFrom() / iTargetWidth, 
                        targetPiece.GetTo() / iTargetWidth ) );
            }
        }

        // Record basic target information:
        record.SetTargetLocation( *pTargetId, targetStrand );

        // Add scores, if available:
        if ( ds.IsSetScores() ) {
            ITERATE ( CDense_seg::TScores, score_it, ds.GetScores() ) {
                record.SetScore( **score_it );
            }
        }
        if ( bInvertWidth ) {
//            record.InvertWidth( 0 );
        }
        x_WriteAlignment( record );
    }
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Writer::WriteHeader()
//  ----------------------------------------------------------------------------
{
    m_Os << "##gff-version 3" << endl;
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Writer::x_WriteBioseqHandle(
    CBioseq_Handle bsh ) 
//  ----------------------------------------------------------------------------
{
    SAnnotSelector sel = x_GetAnnotSelector();
    CFeat_CI feat_iter(bsh, sel);
    feature::CFeatTree feat_tree( feat_iter );

    CGffFeatureContext fc(feature::CFeatTree(feat_iter), bsh);

    CSeqdesc_CI sdi( bsh.GetParentEntry(), CSeqdesc::e_Source, 0 );
    if ( sdi ) {
        CGff3WriteRecordFeature src_feat( 
            fc, 
            string( "id" ) + NStr::UIntToString( m_uPendingGenericId++ ) );
        src_feat.AssignSource( bsh, *sdi );
        x_WriteRecord( &src_feat );
    }

    for ( ;  feat_iter;  ++feat_iter ) {
        
        if ( ! x_WriteFeature( fc, *feat_iter ) ) {
            return false;
        }
    }    
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Writer::x_WriteFeature(
    CGffFeatureContext& fc,
    CMappedFeat mf )
//  ----------------------------------------------------------------------------
{
    if (mf.GetFeatType() == CSeqFeatData::e_Rna) {
        return x_WriteFeatureRna( fc, mf );
    }
    switch( mf.GetFeatSubtype() ) {
        default:
            return x_WriteFeatureGeneric( fc, mf );
        case CSeqFeatData::eSubtype_gene: 
            return x_WriteFeatureGene( fc, mf );
        case CSeqFeatData::eSubtype_cdregion:
            return x_WriteFeatureCds( fc, mf );
    }
    return false;
}

//  ----------------------------------------------------------------------------
bool CGff3Writer::x_WriteFeatureGene(
    CGffFeatureContext& fc,
    CMappedFeat mf )
//  ----------------------------------------------------------------------------
{
    CRef<CGff3WriteRecordFeature> pRecord( 
        new CGff3WriteRecordFeature( 
            fc,
            string( "gene" ) + NStr::UIntToString( m_uPendingGeneId++ ) ) );

    if ( ! pRecord->AssignFromAsn( mf ) ) {
        return false;
    }
    m_GeneMap[ mf ] = pRecord;
    return x_WriteRecord( pRecord );
}

//  ----------------------------------------------------------------------------
bool CGff3Writer::x_WriteFeatureCds(
    CGffFeatureContext& fc,
    CMappedFeat mf )
//  ----------------------------------------------------------------------------
{
    CRef< CGff3WriteRecordFeature > pCds( new CGff3WriteRecordFeature( fc ) );

    if ( ! pCds->AssignFromAsn( mf ) ) {
        return false;
    }
    CMappedFeat mrna = feature::GetBestMrnaForCds( mf, &fc.FeatTree() );
    TMrnaMap::iterator it = m_MrnaMap.find( mrna );
    if ( it != m_MrnaMap.end() ) {
        pCds->AssignParent( *(it->second) );
    }

    const CSeq_loc& PackedInt = *pCds->GetCircularLocation();

    unsigned int uTotSize = 0;
    unsigned int /*CCdregion::EFrame*/ uInitFrame = 0;
    if (mf.GetData().GetCdregion().IsSetFrame()) {
        uInitFrame = mf.GetData().GetCdregion().GetFrame();
    }
    if ( PackedInt.IsPacked_int() && PackedInt.GetPacked_int().CanGet() ) {
        const list< CRef< CSeq_interval > >& sublocs = PackedInt.GetPacked_int().Get();
        list< CRef< CSeq_interval > >::const_iterator it;
        for ( it = sublocs.begin(); it != sublocs.end(); ++it ) {
            const CSeq_interval& subint = **it;
            CRef<CGff3WriteRecordFeature> pExon( new CGff3WriteRecordFeature( *pCds ) );
            pExon->CorrectType( "CDS" );
            pExon->CorrectLocation( subint );
            pExon->CorrectPhase( uTotSize, uInitFrame );
            pExon->ForceAttributeID( string( "cds" ) + NStr::UIntToString( m_uPendingCdsId ) );
            if ( ! x_WriteRecord( pExon ) ) {
                return false;
            }
            uTotSize += subint.GetLength();
        }
    }
    ++m_uPendingCdsId;
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Writer::x_WriteFeatureRna(
    CGffFeatureContext& fc,
    CMappedFeat mf )
//  ----------------------------------------------------------------------------
{
    CRef<CGff3WriteRecordFeature> pRna( 
        new CGff3WriteRecordFeature( 
            fc,
            string( "rna" ) + NStr::UIntToString( m_uPendingTrnaId++ ) ) );
    if ( ! pRna->AssignFromAsn( mf ) ) {
        return false;
    }
    CMappedFeat gene = feature::GetBestGeneForFeat( mf, &fc.FeatTree() );
    TGeneMap::iterator it = m_GeneMap.find( gene );
    if ( it != m_GeneMap.end() ) {
        pRna->AssignParent( *( it->second ) );
    }
    if ( ! x_WriteRecord( pRna ) ) {
        return false;
    }
    if (mf.GetFeatSubtype() == CSeqFeatData::eSubtype_mRNA) {
        m_MrnaMap[ mf ] = pRna;
    }    

    const CSeq_loc& PackedInt = *pRna->GetCircularLocation();

    string strExonId = string("id") + NStr::UIntToString(m_uPendingGenericId++);
    if ( PackedInt.IsPacked_int() && PackedInt.GetPacked_int().CanGet() ) {
        const list< CRef< CSeq_interval > >& sublocs = PackedInt.GetPacked_int().Get();
        list< CRef< CSeq_interval > >::const_iterator it;
        for ( it = sublocs.begin(); it != sublocs.end(); ++it ) {
            const CSeq_interval& subint = **it;
            CRef<CGff3WriteRecordFeature> pChild( 
                new CGff3WriteRecordFeature( *pRna ) );
            pChild->CorrectType("exon");
            pChild->AssignParent(*pRna);
            pChild->CorrectLocation( subint );
            pChild->ForceAttributeID(strExonId);
            if ( ! x_WriteRecord( pChild ) ) {
                return false;
            }
        }
        return true;
    }
    return true;    
}

//  ----------------------------------------------------------------------------
bool CGff3Writer::x_WriteFeatureGeneric(
    CGffFeatureContext& fc,
    CMappedFeat mf )
//  ----------------------------------------------------------------------------
{
    CRef<CGff3WriteRecordFeature> pParent( new CGff3WriteRecordFeature( fc ) );
    if ( ! pParent->AssignFromAsn( mf ) ) {
        return false;
    }
    string strId;
    if ( ! pParent->GetAttribute( "ID", strId ) ) {
        pParent->ForceAttributeID( 
            string( "id" ) + NStr::UIntToString( m_uPendingGenericId++ ) );
    }

    //
    //  Even for generic features, there are special case to consider ...
    //
    if (mf.GetData().GetSubtype() == CSeqFeatData::eSubtype_exon) {
        CMappedFeat gene = feature::GetBestGeneForFeat( mf, &fc.FeatTree() );
        TGeneMap::iterator it = m_GeneMap.find( gene );
        if ( it != m_GeneMap.end() ) {
            pParent->AssignParent( *( it->second ) );
        }
    }

    CRef< CSeq_loc > pPackedInt( new CSeq_loc( CSeq_loc::e_Mix ) );
    pPackedInt->Add( mf.GetLocation() );
    pPackedInt->ChangeToPackedInt();

    if ( pPackedInt->IsPacked_int() && pPackedInt->GetPacked_int().CanGet() ) {
        const list< CRef< CSeq_interval > >& sublocs = pPackedInt->GetPacked_int().Get();
        list< CRef< CSeq_interval > >::const_iterator it;
        for ( it = sublocs.begin(); it != sublocs.end(); ++it ) {
            const CSeq_interval& subint = **it;
            CRef<CGff3WriteRecordFeature> pChild( 
                new CGff3WriteRecordFeature( *pParent ) );
            pChild->CorrectLocation( subint );
            if ( ! x_WriteRecord( pChild ) ) {
                return false;
            }
        }
        return true;
    }
    
    // default behavior:
    return x_WriteRecord( pParent );    
}

//  ============================================================================
void CGff3Writer::x_WriteAlignment( 
    const CGffAlignmentRecord& record )
//  ============================================================================
{
    m_Os << record.StrId() << '\t';
    m_Os << record.StrSource() << '\t';
    m_Os << record.StrType() << '\t';
    m_Os << record.StrSeqStart() << '\t';
    m_Os << record.StrSeqStop() << '\t';
    m_Os << record.StrScore() << '\t';
    m_Os << record.StrStrand() << '\t';
    m_Os << record.StrPhase() << '\t';
    m_Os << record.StrAttributes() << endl;
}

END_NCBI_SCOPE
