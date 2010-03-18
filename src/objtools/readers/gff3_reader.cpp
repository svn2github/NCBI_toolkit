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
 * Author:  Frank Ludwig
 *
 * File Description:
 *   GFF file reader
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbithr.hpp>
#include <corelib/ncbiutil.hpp>
#include <corelib/ncbiexpt.hpp>
#include <corelib/stream_utils.hpp>

#include <util/static_map.hpp>
#include <util/line_reader.hpp>

#include <serial/iterator.hpp>
#include <serial/objistrasn.hpp>

// Objects includes
#include <objects/general/Int_fuzz.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/general/User_object.hpp>
#include <objects/general/User_field.hpp>
#include <objects/general/Dbtag.hpp>

#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <objects/seqloc/Seq_point.hpp>

#include <objects/seq/Seq_annot.hpp>
#include <objects/seq/Annot_id.hpp>
#include <objects/seq/Annotdesc.hpp>
#include <objects/seq/Annot_descr.hpp>
#include <objects/seq/Seq_descr.hpp>
#include <objects/seqfeat/SeqFeatData.hpp>
#include <objects/seqfeat/SeqFeatXref.hpp>

#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqfeat/BioSource.hpp>
#include <objects/seqfeat/Org_ref.hpp>
#include <objects/seqfeat/OrgName.hpp>
#include <objects/seqfeat/SubSource.hpp>
#include <objects/seqfeat/OrgMod.hpp>
#include <objects/seqfeat/Gene_ref.hpp>
#include <objects/seqfeat/Code_break.hpp>
#include <objects/seqfeat/Genetic_code.hpp>
#include <objects/seqfeat/Genetic_code_table.hpp>
#include <objects/seqfeat/RNA_ref.hpp>
#include <objects/seqfeat/Trna_ext.hpp>
#include <objects/seqfeat/Imp_feat.hpp>
#include <objects/seqfeat/Gb_qual.hpp>
#include <objects/seqfeat/Feat_id.hpp>
#include <objects/seqset/Bioseq_set.hpp>

#include <objtools/readers/reader_exception.hpp>
#include <objtools/readers/line_error.hpp>
#include <objtools/readers/error_container.hpp>
#include <objtools/readers/gff3_sofa.hpp>
#include <objtools/readers/gff3_reader.hpp>
#include <objtools/readers/gff3_data.hpp>
#include <objtools/error_codes.hpp>

#include <algorithm>

#include "gff3_data.hpp"

#define NCBI_USE_ERRCODE_X   Objtools_Rd_RepMask

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

//  ----------------------------------------------------------------------------
bool s_GetAnnotId(
    const CSeq_annot& annot,
    string& strId )
//  ----------------------------------------------------------------------------
{
    if ( ! annot.CanGetId() || annot.GetId().size() != 1 ) {
        // internal error
        return false;
    }
    
    CRef< CAnnot_id > pId = *( annot.GetId().begin() );
    if ( ! pId->IsLocal() ) {
        // internal error
        return false;
    }
    strId = pId->GetLocal().GetStr();
    return true;
}

//  ----------------------------------------------------------------------------
CGff3Reader::CGff3Reader(
    unsigned int uFlags ):
//  ----------------------------------------------------------------------------
    m_uFlags( TFlags( uFlags ) ),
    m_pErrors( 0 )
{
}

//  ----------------------------------------------------------------------------
CGff3Reader::~CGff3Reader()
//  ----------------------------------------------------------------------------
{
}


//  --------------------------------------------------------------------------- 
void
CGff3Reader::ReadSeqAnnots(
    vector< CRef<CSeq_annot> >& annots,
    CNcbiIstream& istr,
    IErrorContainer* pErrorContainer )
//  ---------------------------------------------------------------------------
{
    CStreamLineReader lr( istr );
    ReadSeqAnnots( annots, lr, pErrorContainer );
}

//  ---------------------------------------------------------------------------                       
void
CGff3Reader::ReadSeqAnnots(
    vector< CRef<CSeq_annot> >& annots,
    ILineReader& lr,
    IErrorContainer* pErrorContainer )
//  ----------------------------------------------------------------------------
{
    if ( m_uFlags & fNewCode ) {
        return ReadSeqAnnotsNew( annots, lr, pErrorContainer );
    }
    CRef< CSeq_entry > entry = ReadSeqEntry( lr, pErrorContainer );
    CTypeIterator<CSeq_annot> annot_iter( *entry );
    for ( ;  annot_iter;  ++annot_iter) {
        annots.push_back( CRef<CSeq_annot>( annot_iter.operator->() ) );
    }
}
 
//  ---------------------------------------------------------------------------                       
void
CGff3Reader::ReadSeqAnnotsNew(
    vector< CRef<CSeq_annot> >& annots,
    ILineReader& lr,
    IErrorContainer* pErrorContainer )
//  ----------------------------------------------------------------------------
{
    string line;
    int linecount = 0;

    while ( ! lr.AtEOF() ) {
        ++linecount;
        line = *++lr;
        if ( NStr::TruncateSpaces( line ).empty() ) {
            continue;
        }
        try {
            if ( x_IsCommentLine( line ) ) {
                continue;
            }
            if ( x_ParseStructuredCommentGff( line, annots ) ) {
                continue;
            }
            if ( x_ParseBrowserLineGff( line, annots ) ) {
                continue;
            }
            if ( x_ParseTrackLineGff( line, annots ) ) {
                continue;
            }
            x_ParseFeatureGff( line, annots );
        }
        catch( CObjReaderLineException& err ) {
            err.SetLineNumber( linecount );
//            x_ProcessError( err, pErrorContainer );
        }
        continue;
    }
//    if ( m_iFlags & fDumpStats ) {
//        x_DumpStats( cerr );
//    }
    x_AddConversionInfoGff( annots, &m_ErrorsPrivate );
}

//  ----------------------------------------------------------------------------                
CRef< CSeq_entry >
CGff3Reader::ReadSeqEntry(
    ILineReader& lr,
    IErrorContainer* pErrorContainer ) 
//  ----------------------------------------------------------------------------                
{ 
    m_pErrors = pErrorContainer;
//    CRef< CSeq_entry > entry = CGFFReader::Read( lr, m_iReaderFlags );
//    m_pErrors = 0;
//    return entry;

    x_Reset();
    m_TSE->SetSet();
    
    string strLine;
    while( x_ReadLine( lr, strLine ) ) {
    
        try {
            if ( x_ParseStructuredComment( strLine ) ) {
                continue;
            }
            if ( x_ParseBrowserLineToSeqEntry( strLine, m_TSE ) ) {
                continue;
            }
            if ( x_ParseTrackLineToSeqEntry( strLine, m_TSE ) ) {
                continue;
            }
            // -->
            CRef<SRecord> record = x_ParseFeatureInterval(strLine);
            if (record) {
                
                record->line_no = m_LineNumber;
                string id = x_FeatureID(*record);
                record->id = id;

                if (id.empty()) {
                    x_ParseAndPlace(*record);
                } else {
                    CRef<SRecord>& match = m_DelayedRecords[id];
                    // _TRACE(id << " -> " << match.GetPointer());
                    if (match) {
                        x_MergeRecords(*match, *record);
                    } else {
                        match.Reset(record);
                    }
                }
            }
        }
        catch( CObjReaderLineException& err ) {
            ProcessError( err, pErrorContainer );
        }
        // <--
    }
    // -->
    NON_CONST_ITERATE (TDelayedRecords, it, m_DelayedRecords) {
        SRecord& rec = *it->second;
        /// merge mergeable ranges
        NON_CONST_ITERATE (SRecord::TLoc, loc_iter, rec.loc) {
            ITERATE (set<TSeqRange>, src_iter, loc_iter->merge_ranges) {
                TSeqRange range(*src_iter);
                set<TSeqRange>::iterator dst_iter =
                    loc_iter->ranges.begin();
                for ( ;  dst_iter != loc_iter->ranges.end();  ) {
                    TSeqRange r(range);
                    r += *dst_iter;
                    if (r.GetLength() <=
                        range.GetLength() + dst_iter->GetLength()) {
                        range += *dst_iter;
                        _TRACE("merging overlapping ranges: "
                               << range.GetFrom() << " - "
                               << range.GetTo() << " <-> "
                               << dst_iter->GetFrom() << " - "
                               << dst_iter->GetTo());
                        loc_iter->ranges.erase(dst_iter++);
                        break;
                    } else {
                        ++dst_iter;
                    }
                }
                loc_iter->ranges.insert(range);
            }
        }

        if (rec.key == "exon") {
            rec.key = "mRNA";
        }
        x_ParseAndPlace(rec);
    }

    x_RemapGeneRefs( m_TSE, m_GeneRefs );

    CRef<CSeq_entry> tse(m_TSE); // need to save before resetting.
    x_Reset();
    
    // promote transcript_id and protein_id to products
    if ( m_uFlags & fSetProducts ) {
        x_SetProducts( tse );
    }

    if ( m_uFlags & fCreateGeneFeats ) {
        x_CreateGeneFeatures( tse );
    }
    // <--
    x_AddConversionInfo( tse, pErrorContainer );
    return tse;
}
    
//  ----------------------------------------------------------------------------                
CRef< CSerialObject >
CGff3Reader::ReadObject(
    ILineReader& lr,
    IErrorContainer* pErrorContainer ) 
//  ----------------------------------------------------------------------------                
{ 
//    m_pErrors = pErrorContainer;
//    CRef< CSeq_entry > entry = CGFFReader::Read( lr, m_iReaderFlags );
//    m_pErrors = 0;
//    CRef<CSerialObject> object( entry.ReleaseOrNull() );
    CRef<CSerialObject> object( 
        ReadSeqEntry( lr, pErrorContainer ).ReleaseOrNull() );
    return object;
}
    
//  ----------------------------------------------------------------------------                
void 
CGff3Reader::x_Info(
    const string& message,
    unsigned int line )
//  ----------------------------------------------------------------------------                
{
    if ( !m_pErrors ) {
        return CGFFReader::x_Info( message, line );
    }
    CObjReaderLineException err( eDiag_Info, line, message );
    CReaderBase::m_uLineNumber = line;
    ProcessError( err, m_pErrors );
}

//  ----------------------------------------------------------------------------                
void 
CGff3Reader::x_Warn(
    const string& message,
    unsigned int line )
//  ----------------------------------------------------------------------------                
{
    if ( !m_pErrors ) {
        return CGFFReader::x_Warn( message, line );
    }
    CObjReaderLineException err( eDiag_Warning, line, message );
    CReaderBase::m_uLineNumber = line;
    ProcessError( err, m_pErrors );
}

//  ----------------------------------------------------------------------------                
void 
CGff3Reader::x_Error(
    const string& message,
    unsigned int line )
//  ----------------------------------------------------------------------------                
{
    if ( !m_pErrors ) {
        return CGFFReader::x_Error( message, line );
    }
    CObjReaderLineException err( eDiag_Error, line, message );
    CReaderBase::m_uLineNumber = line;
    ProcessError( err, m_pErrors );
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_ReadLine(
    ILineReader& lr,
    string& strLine )
//  ----------------------------------------------------------------------------
{
    strLine.clear();
    while ( ! lr.AtEOF() ) {
        strLine = *++lr;
        ++m_uLineNumber;
        NStr::TruncateSpacesInPlace( strLine );
        if ( ! x_IsCommentLine( strLine ) ) {
            return true;
        }
    }
    return false;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_IsCommentLine(
    const string& strLine )
//  ----------------------------------------------------------------------------
{
    if ( strLine.empty() ) {
        return true;
    }
    return (strLine[0] == '#' && strLine[1] != '#');
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_ParseBrowserLineToSeqEntry(
    const string& strLine,
    CRef<CSeq_entry>& entry )
//  ----------------------------------------------------------------------------
{
    if ( ! NStr::StartsWith( strLine, "browser" ) ) {
        return false;
    }
    CSeq_descr& descr = entry->SetDescr();
    
    vector<string> fields;
    NStr::Tokenize( strLine, " \t", fields, NStr::eMergeDelims );
    for ( vector<string>::iterator it = fields.begin(); it != fields.end(); ++it ) {
        if ( *it == "position" ) {
            ++it;
            if ( it == fields.end() ) {
                CObjReaderLineException err(
                    eDiag_Error,
                    0,
                    "Bad browser line: incomplete position directive" );
                throw( err );
            }
            CRef<CSeqdesc> region( new CSeqdesc() );
            region->SetRegion( *it );
            descr.Set().push_back( region );
            continue;
        }
    }

    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_ParseTrackLineToSeqEntry(
    const string& strLine,
    CRef<CSeq_entry>& entry )
//  ----------------------------------------------------------------------------
{
    if ( ! NStr::StartsWith( strLine, "track" ) ) {
        return false;
    }
    CSeq_descr& descr = entry->SetDescr();

    CRef<CUser_object> trackdata( new CUser_object() );
    trackdata->SetType().SetStr( "Track Data" );    
    
    map<string, string> values;
    x_GetTrackValues( strLine, values );
    for ( map<string,string>::iterator it = values.begin(); it != values.end(); ++it ) {
        x_SetTrackDataToSeqEntry( entry, trackdata, it->first, it->second );
    }
    if ( trackdata->CanGetData() && ! trackdata->GetData().empty() ) {
        CRef<CSeqdesc> user( new CSeqdesc() );
        user->SetUser( *trackdata );
        descr.Set().push_back( user );
    }
    return true;
}

//  ----------------------------------------------------------------------------
void CGff3Reader::x_SetTrackDataToSeqEntry(
    CRef<CSeq_entry>& entry,
    CRef<CUser_object>& trackdata,
    const string& strKey,
    const string& strValue )
//  ----------------------------------------------------------------------------
{
    CSeq_descr& descr = entry->SetDescr();

    if ( strKey == "name" ) {
        CRef<CSeqdesc> name( new CSeqdesc() );
        name->SetName( strValue );
        descr.Set().push_back( name );
        return;
    }
    if ( strKey == "description" ) {
        CRef<CSeqdesc> title( new CSeqdesc() );
        title->SetTitle( strValue );
        descr.Set().push_back( title );
        return;
    }
    trackdata->AddField( strKey, strValue );
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_ParseStructuredCommentGff(
    const string& strLine,
    TAnnots& annots )
//  ----------------------------------------------------------------------------
{
    if ( ! NStr::StartsWith( strLine, "##" ) ) {
        return false;
    }
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_ParseFeatureGff(
    const string& strLine,
    TAnnots& annots )
//  ----------------------------------------------------------------------------
{
    //
    //  Parse the record and determine which ID the given feature will pertain 
    //  to:
    //
    CGff3Record record;
    if ( ! record.AssignFromGff( strLine ) ) {
        return false;
    }

    //
    //  Search annots for a pre-existing annot pertaining to the same ID:
    //
    TAnnotIt it = annots.begin();
    for ( /*NOOP*/; it != annots.end(); ++it ) {
        string strAnnotId;
        if ( ! s_GetAnnotId( **it, strAnnotId ) ) {
            return false;
        }
        if ( record.Id() == strAnnotId ) {
            break;
        }
    }

    //
    //  If a preexisting annot was found, update it with the new feature
    //  information:
    //
    if ( it != annots.end() ) {
        if ( ! x_UpdateAnnot( record, *it ) ) {
            return false;
        }
    }

    //
    //  Otherwise, create a new annot pertaining to the new ID and initialize it
    //  with the given feature information:
    //
    else {
        CRef< CSeq_annot > pAnnot( new CSeq_annot );
        if ( ! x_InitAnnot( record, pAnnot ) ) {
            return false;
        }
        annots.push_back( pAnnot );      
    }
 
    return true; 
};

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_ParseBrowserLineGff(
    const string& strRawInput,
    TAnnots& )
//  ----------------------------------------------------------------------------
{ 
    if ( NStr::StartsWith( strRawInput, "browser" ) ) {
        return true;
    }
    return false; 
};

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_ParseTrackLineGff(
    const string& strRawInput,
    TAnnots& )
//  ----------------------------------------------------------------------------
{ 
    if ( NStr::StartsWith( strRawInput, "track" ) ) {
        return true;
    }
    return false; 
};
                                
//  ----------------------------------------------------------------------------
void CGff3Reader::x_AddConversionInfoGff(
    TAnnots&,
    IErrorContainer* )
//  ----------------------------------------------------------------------------
{
}                    

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_InitAnnot(
    const CGff3Record& gff,
    CRef< CSeq_annot > pAnnot )
//  ----------------------------------------------------------------------------
{
    CRef< CAnnot_id > pAnnotId( new CAnnot_id );
    pAnnotId->SetLocal().SetStr( gff.Id() );
    pAnnot->SetId().push_back( pAnnotId );

    return x_UpdateAnnot( gff, pAnnot );
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_UpdateAnnot(
    const CGff3Record& gff,
    CRef< CSeq_annot > pAnnot )
//  ----------------------------------------------------------------------------
{
    CRef< CSeq_feat > pFeature( new CSeq_feat );

    if ( ! x_FeatureSetId( gff, pFeature ) ) {
        return false;
    }
    if ( ! x_FeatureSetXref( gff, pFeature ) ) {
        return false;
    }
    if ( ! x_FeatureSetLocation( gff, pFeature ) ) {
        return false;
    }
    if ( ! x_FeatureSetData( gff, pFeature ) ) {
        return false;
    }
    if ( ! x_FeatureSetGffInfo( gff, pFeature ) ) {
        return false;
    }
    if ( ! x_FeatureSetQualifiers( gff, pFeature ) ) {
        return false;
    }
    
    string strId;
    if ( gff.GetAttribute( "ID", strId ) ) {
        m_MapIdToFeature[ strId ] = pFeature;
    }

    return x_AddFeatureToAnnot( pFeature, pAnnot );
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_FeatureSetId(
    const CGff3Record& record,
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    string strId;
    if ( record.GetAttribute( "ID", strId ) ) {
        pFeature->SetId().SetLocal().SetStr( strId );
    }
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_FeatureSetXref(
    const CGff3Record& record,
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    string strParent;
    if ( ! record.GetAttribute( "Parent", strParent ) ) {
        return true;
    }
    CRef< CFeat_id > pFeatId( new CFeat_id );
    pFeatId->SetLocal().SetStr( strParent );
    CRef< CSeqFeatXref > pXref( new CSeqFeatXref );
    pXref->SetId( *pFeatId );
    
    pFeature->SetXref().push_back( pXref );
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_FeatureSetLocation(
    const CGff3Record& record,
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    CRef< CSeq_id > pId( new CSeq_id );
    pId->SetLocal().SetStr( record.Id() );
    CRef< CSeq_loc > pLocation( new CSeq_loc );
    pLocation->SetInt().SetId( *pId );
    pLocation->SetInt().SetFrom( record.SeqStart() );
    pLocation->SetInt().SetTo( record.SeqStop() );
    if ( record.IsSetStrand() ) {
        pLocation->SetInt().SetStrand( record.Strand() );
    }
    pFeature->SetLocation( *pLocation );

    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_FeatureSetQualifiers(
    const CGff3Record& record,
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    CRef< CGb_qual > pQual( new CGb_qual );
    pQual->SetQual( "gff_source" );
    pQual->SetVal( record.Source() );
    pFeature->SetQual().push_back( pQual );

    pQual.Reset( new CGb_qual );
    pQual->SetQual( "gff_type" );
    pQual->SetVal( record.Type() );
    pFeature->SetQual().push_back( pQual );

    if ( record.IsSetScore() ) {
        pQual.Reset( new CGb_qual );
        pQual->SetQual( "gff_score" );
        pQual->SetVal( NStr::DoubleToString( record.Score() ) );
        pFeature->SetQual().push_back( pQual );
    }

    //
    //  Create GB qualifiers for the record attributes:
    //
    const CGff3Record::TAttributes& attrs = record.Attributes();
    CGff3Record::TAttrCit it = attrs.begin();
    for ( /*NOOP*/; it != attrs.end(); ++it ) {
        pQual.Reset( new CGb_qual );
        pQual->SetQual( it->first );
        pQual->SetVal( it->second );
        pFeature->SetQual().push_back( pQual );
    }    
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_FeatureSetGffInfo(
    const CGff3Record& record,
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    CRef< CUser_object > pGffInfo( new CUser_object );
    pGffInfo->SetType().SetStr( "gff-info" );    
    pGffInfo->AddField( "gff-attributes", record.AttributesLiteral() );
    pGffInfo->AddField( "gff-start", NStr::UIntToString( record.SeqStart() ) );
    pGffInfo->AddField( "gff-stop", NStr::UIntToString( record.SeqStop() ) );
    pGffInfo->AddField( "gff-cooked", string( "false" ) );

    pFeature->SetExts().push_back( pGffInfo );
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_FeatureSetData(
    const CGff3Record& record,
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    //
    //  Do something with the phase information --- but only for CDS features!
    //

    CSeqFeatData::ESubtype iGenbankType = SofaTypes().MapSofaTermToGenbankType(
        record.Type() );

    switch( iGenbankType ) {
    default:
        return x_FeatureSetDataMiscFeature( record, pFeature );

    case CSeqFeatData::eSubtype_cdregion:
        return x_FeatureSetDataCDS( record, pFeature );
    case CSeqFeatData::eSubtype_exon:
        return x_FeatureSetDataExon( record, pFeature );
    case CSeqFeatData::eSubtype_gene:
        return x_FeatureSetDataGene( record, pFeature );
    case CSeqFeatData::eSubtype_mRNA:
        return x_FeatureSetDataMRNA( record, pFeature );
    }    
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_FeatureSetDataGene(
    const CGff3Record& record,
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    CGene_ref& geneRef = pFeature->SetData().SetGene();  
 
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_FeatureSetDataMRNA(
    const CGff3Record& record,
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    CRNA_ref& rnaRef = pFeature->SetData().SetRna();
    rnaRef.SetType( CRNA_ref::eType_mRNA );

    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_FeatureSetDataCDS(
    const CGff3Record& record,
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    CCdregion& cdr = pFeature->SetData().SetCdregion();
    if ( record.IsSetPhase() ) {
        cdr.SetFrame( record.Phase() );  
    }  
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_FeatureSetDataExon(
    const CGff3Record& record,
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    CSeqFeatData& data = pFeature->SetData();
    data.SetImp().SetKey( "exon" );
    
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_FeatureSetDataMiscFeature(
    const CGff3Record& record,
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    CSeqFeatData& data = pFeature->SetData();
    data.SetImp().SetKey( "misc_feature" );
    
    return true;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_GetFeatureById(
    const string & strId, 
    ncbi::CRef<CSeq_feat>& pFeature )
//  ----------------------------------------------------------------------------
{
    map< string, CRef< CSeq_feat > >::iterator it;
    for ( it = m_MapIdToFeature.begin(); it != m_MapIdToFeature.end(); ++it ) {
        if ( it->first == strId ) {
            pFeature = it->second;
            return true;
        }
    }
    return false;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_HasTemporaryLocation(
    const CSeq_feat& feature )
//  ----------------------------------------------------------------------------
{
    if ( ! feature.CanGetExts() ) {
        return false;
    }
    list< CRef< CUser_object > > pExts = feature.GetExts();
    list< CRef< CUser_object > >::iterator it;
    for ( it = pExts.begin(); it != pExts.end(); ++it ) {
        if ( ! (*it)->CanGetType() || ! (*it)->GetType().IsStr() ) {
            continue;
        }
        if ( (*it)->GetType().GetStr() != "gff-info" ) {
            continue;
        }
        if ( ! (*it)->HasField( "gff-cooked" ) ) {
            return false;
        }
        return ( (*it)->GetField( "gff-cooked" ).GetData().GetStr() == "false" );
    }
    return false;
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::IsExon(
    CRef< CSeq_feat > pFeature )
//  ----------------------------------------------------------------------------
{
    if ( ! pFeature->CanGetData() || ! pFeature->GetData().IsImp() ) {
        return false;
    }
    return ( pFeature->GetData().GetImp().GetKey() == "exon" );
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_AddFeatureToAnnot(
    CRef< CSeq_feat > pFeature,
    CRef< CSeq_annot > pAnnot )
//  ----------------------------------------------------------------------------
{
    if ( ! IsExon( pFeature ) ) {
        pAnnot->SetData().SetFtable().push_back( pFeature ) ;
        return true;
    }

    CRef< CSeq_feat > pParent;    
    if ( ! x_GetParentFeature( *pFeature, pParent ) ) {
        pAnnot->SetData().SetFtable().push_back( pFeature ) ;
        return true;
    }

    bool bHasInterestingAttributes = false;
    if ( pFeature->CanGetQual() ) {
        const vector< CRef< CGb_qual > >& quals = pFeature->GetQual();
        vector< CRef< CGb_qual > >::const_iterator it;
        for ( it = quals.begin(); it != quals.end(); ++it ) {
            if ( (*it)->CanGetQual() && (*it)->CanGetVal() ) {
                if ( (*it)->GetQual() != "Parent" ) {
                    bHasInterestingAttributes = true;
                    break;
                }
            }
        }
    }
    if ( bHasInterestingAttributes ) {
        pAnnot->SetData().SetFtable().push_back( pFeature ) ;
    }
    return x_FeatureMergeExon( pFeature, pParent );
}

//  ----------------------------------------------------------------------------
bool CGff3Reader::x_GetParentFeature(
    const CSeq_feat& feature,
    CRef< CSeq_feat >& pParent )
//  ----------------------------------------------------------------------------
{
    if ( ! feature.CanGetQual() ) {
        return false;
    }

    string strParentId;
    vector< CRef< CGb_qual > > quals = feature.GetQual();
    vector< CRef< CGb_qual > >::iterator it;
    for ( it = quals.begin(); it != quals.end(); ++it ) {
        if ( (*it)->CanGetQual() && (*it)->GetQual() == "Parent" ) {
            strParentId = (*it)->GetVal();
            break;
        }
    }
    if ( it == quals.end() ) {
        return false;
    }
    if ( ! x_GetFeatureById( strParentId, pParent ) ) {
        return false;
    }
    return true;
}

//  ---------------------------------------------------------------------------
bool CGff3Reader::x_FeatureMergeExon(
    CRef< CSeq_feat > pExon,
    CRef< CSeq_feat > pFeature )
//  ---------------------------------------------------------------------------
{
    if ( x_HasTemporaryLocation( *pFeature ) ) {
        // start rebuilding parent location from scratch
        pFeature->SetLocation().Assign( pExon->GetLocation() );
        list< CRef< CUser_object > > pExts = pFeature->SetExts();
        list< CRef< CUser_object > >::iterator it;
        for ( it = pExts.begin(); it != pExts.end(); ++it ) {
            if ( ! (*it)->CanGetType() || ! (*it)->GetType().IsStr() ) {
                continue;
            }
            if ( (*it)->GetType().GetStr() != "gff-info" ) {
                continue;
            }
            (*it)->SetField( "gff-cooked" ).SetData().SetStr( "true" );
        }
    }
    else {
        // add exon location to current parent location
        pFeature->SetLocation().Add(  pExon->GetLocation() );
    }

    return true;
}
                                
END_objects_SCOPE
END_NCBI_SCOPE
