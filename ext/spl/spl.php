<?php

/* Standard PHP Library
 *
 * (c) M.Boerger 2003
 */

/*! \brief Interface to foreach() construct
 *
 * Any class that implements this interface can for example be used as 
 * the input parameter to foreach() calls which would normally be an 
 * array.
 *
 * The only thing a class has to do is 
 */
interface spl_iterator {
	
	/*! \brief Create a new iterator
	 *
	 * used for example in foreach() operator.
	 */
	function new_iterator();
}

/*! \brief Simple forward iterator
 *
 * Any class that implements this interface can be used as the
 * return of a foreach interface. And hence the class itself
 * can be used as a parameter to be iterated (normally an array).
 *
 * \code
	class c implements spl_iterator, spl_forward {
		private $num = 0;
		function new_iterator() {
			$this->num = 0;
			return $this;
		}
		function current() {
			return $this->num;
		}
		function next() {
			$this->num++;
		}
		function has_more() {
			return $this->num < 5;
		}
	}
  
	$t = new c();

	foreach($t as $num) {
		echo "$num\n";
	}
   \endcode
 *
 * A very interesting usage scenario are for example database queries.
 * Without this interface you need to do it without foreach or fetch the
 * whole rowset into an array.
 *
 * In the above code the class implements both the foreach and the
 * forward interface. Doing this you cannot have nested foreach calls.
 * If you need this you must split the two parts.
 *
 * \code
	class c implements spl_iterator {
		public $max = 3;
		function new_iterator() {
			return new c_iter($this);
		}
	}
	class c_iter implements spl_forward {
		private $obj;
		private $num = 0;
		function __construct($obj) {
			$this->obj = $obj;
		}
		function current() {
			return $this->num;
		}
		function next() {
			$this->num++;
		}
		function has_more() {
			return $this->num < $this->obj->max;
		}
	}
  
	$t = new c();

	foreach($t as $outer) {
		foreach($t as $inner) {
			echo "$outer,$inner\n";
		}
	}
   \endcode
 *
 * You can also use this interface with the for() construct.
 *
 * \code
	class c implements spl_iterator {
		public $max = 3;
		function new_iterator() {
			return new c_iter($this);
		}
	}
 	class c_iter implements spl_forward {
 		private $obj;
		private $num = 0;
		function __construct($obj) {
			$this->obj = $obj;
		}
		function current() {
			return $this->num;
		}
		function next() {
			$this->num++;
		}
		function has_more() {
			return $this->num < $this->obj->max;
		}
 	}

 	$t = new c();

 	for ($iter = $t->new_iterator(); $iter->has_more(); $iter->next()) {
 		echo $iter->current() . "\n";
 	}
   \endcode
 */
interface spl_forward {
	
	/*! \brief Retrieve the current currentent
	 *
	 * \return \c mixed current element or \c false if no more elements
	 */
	function current();

	/*! \brief Forward to next element.
	 */
	function next();

	/*! \brief Check if more elements are available.
	 *
	 * \return \c bool whether or not more elements are available
	 */
	function has_more();
}

/*! \brief A restartable iterator.
 *
 * This iterator allows you to implement a restartable iterator. That
 * means the iterator can be rewind to the first element after accessing
 * any number of elements.
 *
 * \note If you use sequence in foreach then rewind() will be called
 *       first.
 */
interface spl_sequence extends spl_forward {

	/*! Restart the sequence by positioning it to the first element.
	 */
	function rewind();
}

/*! \brief associative interface
 *
 * This interface allows to implement associative iterators
 * and containers.
 */
interface spl_assoc {
	
	/*! \brief Retrieve the current elements key
	 *
	 * \return \c mixed current key or \c false if no more elements
	 */
	function key();
}

/*! \brief associative foreach() interface
 *
 * This interface extends the forward interface to support keys.
 * With this interface you can do:
 * \code
 	$t = new c();
	foreach($t as $key => $elem).
   \endcode
 */	
interface spl_assoc_forward implements spl_forward, spl_assoc {
}

/*! \brief associative sequence
 */
interface spl_assoc_sequence implements spl_sequence, spl_assoc {
}

/*! \brief array read only access for objects
 */
interface spl_array_read {
	
	/*! Check whether or not the given index exists.
	 * The returned value is interpreted as converted to bool.
	 */
	function exists($index);

	/*! Read the value at position $index.
	 * This function is only beeing called if exists() returns true.
	 */
	function get($index);
}

/*! \brief array read/write access for objects.
 *
 * The following example shows how to use an array_writer:
 * \code 
	class array_emulation implemets spl_array_access {
		private $ar = array();
		function exists($index) {
			return array_key_exists($index, $this->ar);
		}
		function get($index) {
			return $this->ar[$index];
		}
		function set($index, $value) {
			$this->ar[$index] = $value;
		}
	}
   \endcode
 */
interface spl_array_access implements spl_array_read {

	/*! Set the value identified by $index to $value.
	 */
	function set($value, $index);
}

?>