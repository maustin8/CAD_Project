Network implementations
=======================

All network implementations are located in `mockturtle/networks/`:

**Headers**

* AIG network: ``mockturtle/networks/aig.hpp``
* MIG network: ``mockturtle/networks/mig.hpp``
* *k*-LUT network: ``mockturtle/networks/klut.hpp``

+-------------------------+-------------+-------------+-----------------+
| Interface method        | AIG network | MIG network | *k*-LUT network |
+=========================+=============+=============+=================+
|                         | *Primary I/O and constants*                 |
+-------------------------+-------------+-------------+-----------------+
| ``get_constant``        | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``create_pi``           | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``create_po``           | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``is_constant``         | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``is_pi``               | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``constant_value``      | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
|                         | *Create unary functions*                    |
+-------------------------+-------------+-------------+-----------------+
| ``create_buf``          | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``create_not``          | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
|                         | *Create binary functions*                   |
+-------------------------+-------------+-------------+-----------------+
| ``create_and``          | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``create_nand``         | ✓           | ✓           |                 |
+-------------------------+-------------+-------------+-----------------+
| ``create_or``           | ✓           | ✓           |                 |
+-------------------------+-------------+-------------+-----------------+
| ``create_nor``          | ✓           | ✓           |                 |
+-------------------------+-------------+-------------+-----------------+
| ``create_lt``           |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``create_le``           |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``create_gt``           |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``create_ge``           |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``create_xor``          | ✓           | ✓           |                 |
+-------------------------+-------------+-------------+-----------------+
| ``create_xnor``         | ✓           |             |                 |
+-------------------------+-------------+-------------+-----------------+
|                         | *Create ternary functions*                  |
+-------------------------+-------------+-------------+-----------------+
| ``create_maj``          |             | ✓           |                 |
+-------------------------+-------------+-------------+-----------------+
| ``create_ite``          |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
|                         | *Create arbitrary functions*                |
+-------------------------+-------------+-------------+-----------------+
| ``create_node``         |             |             | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``clone_node``          | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
|                         | *Restructuring*                             |
+-------------------------+-------------+-------------+-----------------+
| ``substitute_node``     |             | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
|                         | *Structural properties*                     |
+-------------------------+-------------+-------------+-----------------+
| ``size``                | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``num_pis``             | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``num_pos``             | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``num_gates``           | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``fanin_size``          | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``fanout_size``         | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``depth``               |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``level``               |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``is_maj``              |             | ✓           |                 |
+-------------------------+-------------+-------------+-----------------+
| ``is_ite``              |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
|                         | *Functional properties*                     |
+-------------------------+-------------+-------------+-----------------+
| ``node_function``       | ✓           |             |                 |
+-------------------------+-------------+-------------+-----------------+
|                         | *Nodes and signals*                         |
+-------------------------+-------------+-------------+-----------------+
| ``get_node``            | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``make_signal``         | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``is_complemented``     | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``node_to_index``       | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``index_to_node``       | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
|                         | *Node and signal iterators*                 |
+-------------------------+-------------+-------------+-----------------+
| ``foreach_node``        | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``foreach_pi``          | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``foreach_po``          | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``foreach_gate``        | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``foreach_fanin``       | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``foreach_parent``      |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
|                         | *Simulate values*                           |
+-------------------------+-------------+-------------+-----------------+
| ``compute``             | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
|                         | *Mapping*                                   |
+-------------------------+-------------+-------------+-----------------+
| ``has_mapping``         |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``is_cell_root``        |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``clear_mapped``        |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``num_cells``           |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``add_to_mapping``      |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``remove_from_mapping`` |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``cell_function``       |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``set_cell_function``   |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
| ``foreach_cell_fanin``  |             |             |                 |
+-------------------------+-------------+-------------+-----------------+
|                         | *Custom node values*                        |
+-------------------------+-------------+-------------+-----------------+
| ``clear_values``        | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``value``               | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``set_value``           | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``incr_value``          | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``decr_value``          | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
|                         | *Visited flags*                             |
+-------------------------+-------------+-------------+-----------------+
| ``clear_visited``       | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``visited``             | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
| ``set_visited``         | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
|                         | *General methods*                           |
+-------------------------+-------------+-------------+-----------------+
| ``update``              | ✓           | ✓           | ✓               |
+-------------------------+-------------+-------------+-----------------+
