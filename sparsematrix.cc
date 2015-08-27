#include "sparsematrix.h"

namespace L3D
{
    //------------------------------------------------------------------------------
    SparseMatrix::SparseMatrix(std::list<float4>& entries, const unsigned int num_rows_cols,
                               const float normalization_factor,
                               const bool sort_by_row, const bool already_sorted)
    {
        // init
        entries_ = NULL;
        start_indices_ = NULL;
        row_sorted_ = sort_by_row;
        num_rows_cols_ = num_rows_cols;
        num_entries_ = entries.size();

        if(entries.size() == 0 || num_rows_cols == 0)
            return;

        // sort data
        if(!already_sorted)
        {
            if(sort_by_row)
                entries.sort(L3D::sortAffEntriesByRow);
            else
                entries.sort(L3D::sortAffEntriesByCol);
        }

        // copy data
        entries_ = new L3D::DataArray<float4>(entries.size(),1);
        start_indices_ = new L3D::DataArray<int>(num_rows_cols_,1);
        start_indices_->setValue(-1);
        std::list<float4>::iterator it = entries.begin();
        unsigned int pos = 0;
        int current_rc = -1;
        for(; it!=entries.end(); ++it,++pos)
        {
            // store data
            float4 data = *it;
            data.z /= normalization_factor;
            entries_->dataCPU(pos,0)[0] = data;

            // check for new row/column
            int rc;
            if(sort_by_row)
                rc = int((*it).x);
            else
                rc = int((*it).y);

            if(current_rc != rc)
            {
                start_indices_->dataCPU(rc,0)[0] = pos;
                current_rc = rc;
            }
        }

        // copy to GPU
        entries_->upload();
        start_indices_->upload();
    }

    //------------------------------------------------------------------------------
    SparseMatrix::SparseMatrix(std::list<L3D::CLEdge>& entries, const unsigned int num_rows_cols,
                               const float normalization_factor,
                               const bool sort_by_row, const bool already_sorted)
    {
        // init
        entries_ = NULL;
        start_indices_ = NULL;
        row_sorted_ = sort_by_row;
        num_rows_cols_ = num_rows_cols;
        num_entries_ = entries.size();

        if(entries.size() == 0 || num_rows_cols == 0)
            return;

        // sort data
        if(!already_sorted)
        {
            if(sort_by_row)
                entries.sort(L3D::sortCLEdgesByRow);
            else
                entries.sort(L3D::sortCLEdgesByCol);
        }

        // copy data
        entries_ = new L3D::DataArray<float4>(entries.size(),1);
        start_indices_ = new L3D::DataArray<int>(num_rows_cols_,1);
        start_indices_->setValue(-1);
        std::list<CLEdge>::iterator it = entries.begin();
        unsigned int pos = 0;
        int current_rc = -1;
        for(; it!=entries.end(); ++it,++pos)
        {
            // store data
            CLEdge data = *it;
            data.w_ /= normalization_factor;
            entries_->dataCPU(pos,0)[0] = make_float4(data.i_,data.j_,data.w_,0.0f);

            // check for new row/column
            int rc;
            if(sort_by_row)
                rc = (*it).i_;
            else
                rc = (*it).j_;

            if(current_rc != rc)
            {
                start_indices_->dataCPU(rc,0)[0] = pos;
                current_rc = rc;
            }
        }

        // copy to GPU
        entries_->upload();
        start_indices_->upload();
    }

    //------------------------------------------------------------------------------
    SparseMatrix::SparseMatrix(SparseMatrix* M, const bool change_sorting)
    {
        // init
        entries_ = NULL;
        start_indices_ = NULL;
        num_rows_cols_ = M->num_rows_cols();
        num_entries_ = M->num_entries();

        if(M->num_entries() == 0 || M->num_rows_cols() == 0)
            return;

        if(!change_sorting)
            row_sorted_ = M->isRowSorted();
        else
            row_sorted_ = !M->isRowSorted();

        // copy entries
        entries_ = new L3D::DataArray<float4>(num_entries_,1);
        start_indices_ = new L3D::DataArray<int>(num_rows_cols_,1);
        if(!change_sorting)
        {
            // direct copy
            M->entries()->copyTo(entries_);
            M->start_indices()->copyTo(start_indices_);
        }
        else
        {
            // change sorting
            std::list<CLEdge> entries;
            for(unsigned int i=0; i<M->entries()->width(); ++i)
            {
                CLEdge e;
                e.i_ = M->entries()->dataCPU(i,0)[0].x;
                e.j_ = M->entries()->dataCPU(i,0)[0].y;
                e.w_ = M->entries()->dataCPU(i,0)[0].z;
                entries.push_back(e);
            }

            if(row_sorted_)
                entries.sort(L3D::sortCLEdgesByRow);
            else
                entries.sort(L3D::sortCLEdgesByCol);

            start_indices_->setValue(-1);
            std::list<CLEdge>::iterator it = entries.begin();
            unsigned int pos = 0;
            int current_rc = -1;
            for(; it!=entries.end(); ++it,++pos)
            {
                // store data
                CLEdge data = *it;
                entries_->dataCPU(pos,0)[0] = make_float4(data.i_,data.j_,data.w_,0.0f);

                // check for new row/column
                int rc;
                if(row_sorted_)
                    rc = (*it).i_;
                else
                    rc = (*it).j_;

                if(current_rc != rc)
                {
                    start_indices_->dataCPU(rc,0)[0] = pos;
                    current_rc = rc;
                }
            }
        }

        // copy to GPU
        entries_->upload();
        start_indices_->upload();
    }

    //------------------------------------------------------------------------------
    SparseMatrix::~SparseMatrix()
    {
        // cleanup CPU
        if(entries_ != NULL)
            delete entries_;

        if(start_indices_ != NULL)
            delete start_indices_;
    }
}
