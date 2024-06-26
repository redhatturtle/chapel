module block_partition_iterators {

  use cholesky_execution_config_consts;

  // ===================================================================
  // Iterator to create the 2 x 2 block partitionings of the sequence of
  // reduced matrices that occur during a triangular factorization.
  // ===================================================================

  iter symmetric_2_by_2_block_partition ( idx_range ) {

    // -------------------------------------------------------------------
    // Deliver as ranges the sequence of reduced matrices that arise in a
    // triangular factorization, partitioned into 2 by 2 blocks.
    // The output ranges are (from the diagonal downward):
    //    the rows (or columns) in the top (diagonal) block 
    //                          of the reduced matrix
    //    the rows (or columns) in the remainder of the reduced matrix
    // -------------------------------------------------------------------
    // The block size to be used is delivered via a global constant
    // rather than as an argument to allow this code to emulate a code
    // in which the block size were obtained from a blocking distribution.
    // -------------------------------------------------------------------
    

    for block_low in idx_range by block_size do {
      var next_block_low = block_low + block_size;
      
      yield ( block_low      .. min ( next_block_low - 1, idx_range.high ),
	      next_block_low .. idx_range.high );
    }
  }


  // ====================================
  // Iterator to Block Partition a Vector
  // (or the rows or columns of a Matrix)
  // ====================================

  iter vector_block_partition ( idx_range )
  {
    // -------------------------------------------------------------------
    // Deliver as ranges the block partitioning of a vector.
    // The block size to be used is delivered via a global constant
    // rather than as an argument to allow this code to emulate a code
    // in which the block size were obtained from a blocking distribution.
    // -------------------------------------------------------------------

    for block_low in idx_range by block_size do
      yield block_low .. min ( block_low + block_size - 1, idx_range.high );
  }



    proc block_leading_indices ( matrix_domain )  {

      // -------------------------------------------------------------------
      // Deliver as a strided domain the leading / low indices of the blocks
      // in a 2D partitioning of the matrix. These leading indices, the 
      // "upper left hand corner" indices in each block. are used to index
      // the blocks in the partitioned matrix.
      //
      // The block size to be used is delivered via a global constant
      // rather than as an argument to allow this code to emulate a code
      // in which the block size were obtained from a blocking distribution.
      //
      // For full generality with a ScaLAPACK 2D distribution, it might be
      // necessary to use a sparse subdomain rather than a strided domain,
      // to allow submatrices with odd sized blocks on all sides.
      // -------------------------------------------------------------------

    var block_leading_index_domain : domain (2, strides=strideKind.positive) = 
      matrix_domain by ( block_size: uint, block_size: uint );

    return block_leading_index_domain;
  }
}