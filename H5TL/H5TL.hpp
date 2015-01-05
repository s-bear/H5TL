/**********
 * The MIT License (MIT)
 * 
 * Copyright (c) 2014 Samuel Bear Powell
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
\**********/
#ifndef H5TL_HPP
#define H5TL_HPP
//HDF5:
#include "hdf5.h"
//#include "hdf5_hl.h"
//STL:
#include <utility>
#include <vector>
#include <array>
#include <iterator>
#include <numeric>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <cstdint>

namespace H5TL {
	namespace util {
		template<typename InputIt, typename T>
		T product(InputIt first, InputIt last, T init) {
			return std::accumulate(first, last, init, std::multiplies<T>());
		}

		template<typename T>
		T clip(const T& val, const T& min, const T& max) {
			if (val < min) return min;
			else if (val > max) return max;
			else return val;
		}

		template<typename T>
		void prepend(std::vector<T> &vec, size_t N, const T& t) {
			std::vector<T> tmp(vec.size() + N);
			std::fill_n(tmp.begin(), N, t);
			std::copy_backward(vec.begin(), vec.end(), tmp.end());
			vec = std::move(tmp);
		}

		template<typename InputIt>
		std::string join(const std::string& delim, InputIt first, InputIt last) {
			std::ostringstream builder;
			builder << *first; ++first;
			while (first != last) {
				builder << delim << *first;
				++first;
			}
			return builder.str();
		}
	}
	//General library interface:

	//forware declarations & typedefs
	template<typename XX> class ErrorHandler_;
	typedef ErrorHandler_<void> ErrorHandler;

	class ID;

	template<typename XX> class DType_;
	typedef DType_<void> DType;

	class PDType;

	template<typename data_t, typename enable> struct adapt;

	class Props;
	template<typename XX> class LProps_;
	template<typename XX> class DProps_;
	typedef LProps_<void> LProps;
	typedef DProps_<void> DProps;

	template<typename XX> class DSpace_;
	typedef DSpace_<void> DSpace;

	template<typename XX> class Selection_;
	typedef Selection_<void> Selection;
	class SelectAll;
	class Hyperslab;

	class Attribute;
	class Object;
	class Dataset;
	class Group;
	class File;

	///exception class for all HDF5 errors.
	class h5tl_error : public std::runtime_error {
	public:
		explicit h5tl_error(const std::string& what_arg) : std::runtime_error(what_arg) {}
		explicit h5tl_error(const char* what_arg) : std::runtime_error(what_arg) {}
	};

	///class for walking the HDF5 error stack.
	template<typename XX>
	class ErrorHandler_ {
	protected:
		//use constructor of private static member to run this when the library loads:
		ErrorHandler_() {
			//disable automatic error printing
			H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);
		}
		static std::string class_name(hid_t class_id) {
			ssize_t len = H5Eget_class_name(class_id, nullptr, 0);
			std::string tmp(len, '\0');
			H5Eget_class_name(class_id, &(tmp[0]), len);
			return tmp;
		}
		static herr_t append_error(unsigned n, const H5E_error2_t *err_desc, void *client_data) {
			std::ostream &os = *(std::ostream*)client_data;
			os << "#" << n << ": " << err_desc->file_name << " line " << err_desc->line << ": " << err_desc->func_name << "(): " << std::endl;
			os << "    " << H5Eget_major(err_desc->maj_num) << ": " << H5Eget_minor(err_desc->min_num) << ": " << err_desc->desc << std::endl;
			//os << "class: " << _error_class_name(err_desc->cls_id) << std::endl;
			return 1;
		}
	public:
		/** \brief Return a std::string for the HDF5 error stack.

		Walks and then clears the indicated HDF5 error stack, building a string description of the current error.
		\param[in] es The error stack to walk, and subsequently clear.
		\return A string representation of the error stack.
		*/
		std::string get_error(hid_t es = H5E_DEFAULT) const {
			std::ostringstream oss;
			H5Ewalk2(es, H5E_WALK_DOWNWARD, append_error, &oss);
			H5Eclear2(es);
			return oss.str();
		}
		static const ErrorHandler_<void> EH;
	};
	
	template<typename XX>
	const ErrorHandler ErrorHandler_<XX>::EH;

	/** \brief Checks an HDF5 herr_t return code for errors.
	*
	* Checks an HDF5 herr_t return code for errors. If there is an error, throws an H5TL::h5tl_error with the message given by ErrorHandler::get_error().
	* \param[in] e Error code to check, throws an H5TL::h5tl_error if e < 0.
	*/
	inline void check(herr_t e) {
		if (e < 0) throw h5tl_error(ErrorHandler::EH.get_error());
	}

	/** \brief Checks an HDF5 htri_t return code for errors and converts it to a bool.
	*
	* Checks an HDF5 tri_t return code for errors and converts it to a bool. If there is an error, throws an H5TL::h5tl_error with the message given by ErrorHandler::get_error().
	* \param t The htri_t return code to check
	* \returns true if t > 0
	*/
	inline bool check_tri(htri_t t) {
		if (t < 0) throw h5tl_error(ErrorHandler::EH.get_error());
		else return t > 0;
	}
	/** \brief Checks an HDF5 hid_t return code for errors.
	*
	* Checks an HDF5 hid_t return code for errors and returns it. If there is an error, throws an H5TL::h5tl_error with the message given by ErrorHandler::get_error().
	* \param id The hid_t return code to check.
	* \returns id, unchanged.
	*/
	inline hid_t check_id(hid_t id) {
		if (id < 0) throw h5tl_error(ErrorHandler::EH.get_error());
		else return id;
	}
	/** \brief Checks an HDF5 ssize_t return code for errors.
	*
	* Checks an HDF5 ssize_t return code for errors and returns it. If there is an error, throws an H5TL::h5tl_error with the message given by ErrorHandler::get_error().
	* \param sz The ssize_t return code to check.
	* \returns sz, unchanged.
	*/
	inline ssize_t check_ssize(ssize_t sz) {
		if (sz < 0) throw h5tl_error(ErrorHandler::EH.get_error());
		else return sz;
	}
	/** \brief Checks an HDF5 hssize_t return code for errors.
	*
	* Checks an HDF5 hssize_t return code for errors and returns it. If there is an error, throws an H5TL::h5tl_error with the message given by ErrorHandler::get_error().
	* \param sz The hssize_t return code to check.
	* \returns sz, unchanged.
	*/
	inline hssize_t check_hssize(hssize_t sz) {
		if (sz < 0) throw h5tl_error(ErrorHandler::EH.get_error());
		else return sz;
	}
	/**
	Initializes the HDF5 library.
	*/
	inline void open() {
		check(H5open());
	}
	/**
	Close the HDF5 library.
	*/
	inline void close() {
		check(H5close());
	}

	inline void swap(ID&, ID&);

	/** \brief Virtual class encapsulating an hid_t. */
	class ID {
		friend void swap(ID&, ID&);
	protected:
		hid_t id;
		ID() : id(0) {}
		ID(hid_t _id) : id(check_id(_id)) {}
		//copy constructor is protected - can be made public as necessary by sub-classes
		ID(const ID& i) : id(i.id) {}
		//copy assign is protected - can be made public as necessary by sub-classes
		ID& operator=(const ID& i) {
			if (id) close();
			id = i.id;
			return *this;
		}
		void steal(ID& i) {
			if (id) close();
			id = i.id;
			i.id = 0;
		}
		void swap(ID& i1) {
			std::swap(id, i1.id);
		}
	public:
		//move constructor
		ID(ID &&i) : id(i.id) { i.id = 0; }

		//move assignment
		ID& operator=(ID&& i)  {
			steal(i);
			return *this;
		}

		virtual ~ID() {}
		virtual void close() = 0;
		virtual operator hid_t() const { return id; }
		virtual bool operator==(const ID& other) const {
			return this == &other || id == other.id;
		}
		virtual bool operator<(const ID& other) const {
			return id < other.id;
		}
		/** \brief Checks that this ID refers to a valid HDF5 object.
		* \returns true if this refers to a valid HDF5 object.
		*/
		virtual bool valid() const { return check_tri(H5Iis_valid(id)); }
		virtual operator bool() const { return valid(); }

	};
	inline void swap(ID& i0, ID& i1) {
		i0.swap(i1);
	}

	/** \brief Encapsulate an HDF5 data type object.
	*
	* DetailedDescription
	*/
	template<typename XX>
	class DType_ : public ID {
		friend class Dataset;
		friend class Attribute;
	protected:
		DType_(hid_t id) : ID(id) {}
		DType_(hid_t id, size_t sz) : ID(H5Tcopy(id)) {
			size(sz);
		}
	public:
		DType_() : ID() {}
		//copy
		DType_(const DType_ &dt) : ID(check_id(H5Tcopy(dt))) {}
		DType_& operator=(DType_ dt) {
			swap(dt);
			return *this;
		}

		//move
		DType_(DType_ &&dt) : ID(std::move(dt)) {}
		DType_& operator=(DType_&& dt) {
			steal(dt);
			return *this;
		}
		DType_(const DType_ &dt, size_t sz) : ID(check_id(H5Tcopy(dt))) {
			size(sz);
		}
		virtual ~DType_() {
			if (id) close();
		}
		void close() {
			check(H5Tclose(id)); id = 0;
		}
		/** \brief Set the size, in bytes, of the datatype
		*
		* Set the size, in bytes, of the datatype.
		* \param[in] sz The size of the datatype, in bytes. Set to 0 for variable-size strings.
		*/
		void size(size_t sz) {
			if (sz == 0) check(H5Tset_size(id, H5T_VARIABLE));
			else check(H5Tset_size(id, sz));
		}
		virtual size_t size() const {
			return H5Tget_size(id);
		}
		virtual bool operator==(const DType_& other) const {
			return check_tri(H5Tequal(id, other));
		}
		static const PDType NONE; ///< Not a valid type
		static const PDType INT8; ///< Native 8-bit signed integer type
		static const PDType UINT8; ///< Native 8-bit unsigned integer type
		static const PDType INT16; ///< Native 16-bit signed integer type
		static const PDType UINT16; ///< Native 16-bit unsigned integer type
		static const PDType INT32; ///< Native 32-bit signed integer type
		static const PDType UINT32; ///< Native 32-bit unsigned integer type
		static const PDType INT64; ///< Native 64-bit signed integer type
		static const PDType UINT64; ///< Native 64-bit unsigned integer type
		static const PDType FLOAT; ///< Native float type
		static const PDType DOUBLE; ///< Native double-precision float type
		static const PDType STRING; ///< Native string character type (copy and set size for multi-character strings)
		static const PDType REFERENCE; ///< HDF5 object reference type
	};

	class PDType : public DType {
		friend class DType_<void>;
	protected:
		PDType(hid_t id) : DType(id) {}
	public:
		//copy
		PDType(const PDType& dt) : DType(dt.id) {}
		PDType& operator=(PDType dt) {
			swap(dt);
			return *this;
		}
		//move
		PDType(PDType &&dt) : DType(std::move(dt)) {}
		PDType& operator=(PDType&& dt) {
			steal(dt);
			return *this;
		}
		virtual ~PDType() { id = 0; } //doesn't need to close(), but set id=0 so other destructors don't try to close it!
		void close() { id = 0; }
	};

	template<typename XX> const PDType DType_<XX>::NONE = PDType(0);
	template<typename XX> const PDType DType_<XX>::INT8 = PDType(H5T_NATIVE_INT8);
	template<typename XX> const PDType DType_<XX>::UINT8 = PDType(H5T_NATIVE_UINT8);
	template<typename XX> const PDType DType_<XX>::INT16 = PDType(H5T_NATIVE_INT16);
	template<typename XX> const PDType DType_<XX>::UINT16 = PDType(H5T_NATIVE_UINT16);
	template<typename XX> const PDType DType_<XX>::INT32 = PDType(H5T_NATIVE_INT32);
	template<typename XX> const PDType DType_<XX>::UINT32 = PDType(H5T_NATIVE_UINT32);
	template<typename XX> const PDType DType_<XX>::INT64 = PDType(H5T_NATIVE_INT64);
	template<typename XX> const PDType DType_<XX>::UINT64 = PDType(H5T_NATIVE_UINT64);
	template<typename XX> const PDType DType_<XX>::FLOAT = PDType(H5T_NATIVE_FLOAT);
	template<typename XX> const PDType DType_<XX>::DOUBLE = PDType(H5T_NATIVE_DOUBLE);
	template<typename XX> const PDType DType_<XX>::STRING = PDType(H5T_C_S1);
	template<typename XX> const PDType DType_<XX>::REFERENCE = PDType(H5T_STD_REF_OBJ);

	//template function returns reference to predefined datatype
	template<typename T> const DType& dtype() { static_assert(false, "Type T is not convertible to a H5TL::DType"); }
	//specialization for basic types:
	template<> inline const DType& dtype<int8_t>() { return DType::INT8; }
	template<> inline const DType& dtype<uint8_t>() { return DType::UINT8; }
	template<> inline const DType& dtype<int16_t>() { return DType::INT16; }
	template<> inline const DType& dtype<uint16_t>() { return DType::UINT16; }
	template<> inline const DType& dtype<int32_t>() { return DType::INT32; }
	template<> inline const DType& dtype<uint32_t>() { return DType::UINT32; }
	template<> inline const DType& dtype<int64_t>() { return DType::INT64; }
	template<> inline const DType& dtype<uint64_t>() { return DType::UINT64; }
	template<> inline const DType& dtype<float>() { return DType::FLOAT; }
	template<> inline const DType& dtype<double>() { return DType::DOUBLE; }
	template<> inline const DType& dtype<const char*>() { return DType::STRING; }

	//H5TL adapter traits, enable parameter is for using enable_if for specializations
	//specializations for compatible types are at the end of this file
	template<typename data_t, typename enable = void>
	struct adapt {
		//here are required typedefs and member functions for complete compatibility
		typedef typename std::remove_cv<data_t>::type data_t;
		typedef const DType& dtype_return;
		typedef void* data_return;
		typedef const void* const_data_return;
		typedef data_t allocate_return;
		//default implementation will fail on every call
		static size_t rank(const data_t&) {
			static_assert(false, "rank(const data_t&) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
		}
		static std::vector<hsize_t> shape(const data_t&) {
			static_assert(false, "shape(const data_t&) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
		}
		static dtype_return dtype(const data_t&) {
			static_assert(false, "dtype(const data_t&) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
		}
		static data_return data(data_t&) {
			static_assert(false, "data(data_t&) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
		}
		static const_data_return data(const data_t&) {
			static_assert(false, "data(const data_t&) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
		}
		static allocate_return allocate(const std::vector<hsize_t>&, const DType&) {
			static_assert(false, "allocate<data_t>(...) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
		}
	};

	//functions to automatically infer proper adapter type:
	template<typename data_t>
	size_t rank(const data_t& d) {
		return adapt<data_t>::rank(d);
	}
	template<typename data_t>
	std::vector<hsize_t> shape(const data_t& d) {
		return adapt<data_t>::shape(d);
	}
	template<typename data_t>
	typename adapt<data_t>::dtype_return
		dtype(const data_t& d) {
			return adapt<data_t>::dtype(d);
	}
	template<typename data_t>
	typename adapt<data_t>::data_return
		data(data_t& d) {
			return adapt<data_t>::data(d);
	}
	template<typename data_t>
	typename adapt<data_t>::const_data_return
		data(const data_t& d) {
			return adapt<data_t>::data(d);
	}
	template<typename data_t>
	typename adapt<data_t>::allocate_return
		allocate(const std::vector<hsize_t>& shape, const DType& dt = DType::NONE) {
			return adapt<data_t>::allocate(shape, dt);
	}
	//property list
	class Props : public ID {
	protected:
		Props(hid_t id) : ID(id) {}
		Props(hid_t class_id, int) : ID(H5Pcreate(class_id)) {}
	public:
		//copy
		Props(const Props &p) : ID(H5Pcopy(p)) {}
		Props& operator=(Props p) {
			swap(p);
			return *this;
		}
		//move
		Props(Props &&p) : ID(std::move(p)) {}
		Props& operator=(Props&& p) {
			steal(p);
			return *this;
		}
		virtual ~Props() {
			if (id) close();
		}
		virtual void close() {
			check(H5Pclose(id)); id = 0;
		}
	};

	//link creation properties
	template<typename XX>
	class LProps_ : public Props {
		LProps_(hid_t id) : Props(id) {}
	public:
		static const LProps_<void> DEFAULT;
		LProps_() : Props(H5P_LINK_CREATE, 0) {}
		//copy
		LProps_(const LProps_ &lp) : Props(lp) {}
		LProps_& operator=(LProps_ lp) {
			swap(lp);
			return *this;
		}
		//move
		LProps_(LProps_ &&lp) : Props(std::move(lp)) {}
		LProps_& operator=(LProps_&& lp) {
			steal(lp);
			return *this;
		}
		virtual ~LProps_() {}
		LProps_& create_intermediate() {
			check(H5Pset_create_intermediate_group(id, 1));
			return *this;
		}
	};

	template<typename XX> const LProps LProps_<XX>::DEFAULT = LProps().create_intermediate();

	//dataset creation properties
	template<typename XX>
	class DProps_ : public Props {
		DProps_(hid_t id) : Props(id) {}
	public:
		static const DProps_<void> DEFAULT;
		DProps_() : Props(H5P_DATASET_CREATE, 0) {}
		//copy
		DProps_(const DProps_ &dp) : Props(dp) {}
		DProps_& operator=(DProps_ dp) {
			swap(dp);
			return *this;
		}
		//move
		DProps_(DProps_ &&dp) : Props(std::move(dp)) {}
		DProps_& operator=(DProps_&& dp) {
			steal(dp);
			return *this;
		}
		virtual ~DProps_() {
			if (id) close();
		}
		virtual void close() {
			H5Pclose(id); id = 0;
		}
		//chainable property setters:
		//eg. DProps().chunked().deflate(3).shuffle();
		DProps_& compact() {
			check(H5Pset_layout(id, H5D_COMPACT));
			return *this;
		}
		DProps_& contiguous() {
			check(H5Pset_layout(id, H5D_CONTIGUOUS));
			return *this;
		}
		DProps_& chunked() {
			check(H5Pset_layout(id, H5D_CHUNKED));
			return *this;
		}
		DProps_& chunked(int ndims, const hsize_t *chunk_shape) {
			check(H5Pset_chunk(id, ndims, chunk_shape));
			return *this;
		}
		template<int N>
		DProps_& chunked(const hsize_t(&chunk_shape)[N]) {
			return chunked(N, chunk_shape);
		}
		DProps_& chunked(const std::vector<hsize_t> &chunk_shape) {
			return chunked(int(chunk_shape.size()), chunk_shape.data());
		}
		DProps_& chunked(const std::vector<hsize_t> &data_shape, size_t item_nbytes, size_t chunk_nbytes = 0, size_t line_nbytes = 8192) {
			//replace any 0s in data_shape with 1s
			std::vector<hsize_t> dshape(data_shape);
			for (auto &d : dshape) if (d == 0) d = 1;
			//if chunk_nbytes is 0, we should compute a value for it:
			if (chunk_nbytes == 0) {
				//get size of dataset in mb
				size_t data_mb = (item_nbytes*util::product(dshape.begin(), dshape.end(), size_t(1))) >> 20;
				data_mb = util::clip<size_t>(data_mb, 1, 1 << 23); //stay between 1 MB and 8 TB
				//data size -> chunk size: 1 MB -> 1 line, 1 TB -> 1k lines
				//                         2^0  -> 2^0,   2^20 -> 2^10
				//  chunk_size = 2^(log2(data_mb)*10/20)*line_size = sqrt(data_mb)*line_size
				chunk_nbytes = size_t(sqrt(data_mb))*line_nbytes;
			}
			//given chunk_nbytes, how many items should be in the chunk?
			size_t desired_chunk_size(chunk_nbytes / item_nbytes);
			//start with the chunk size the same as the whole shape:
			std::vector<hsize_t> chunk_shape(dshape);
			//it's possible the shape is <= the desired chunk size
			size_t chunk_size = util::product(chunk_shape.begin(), chunk_shape.end(), size_t(1));
			if (chunk_size <= desired_chunk_size)
				return chunked(chunk_shape);
			//if it's not, trim each dimension in turn to reduce the chunk size
			for (size_t trim_dim = 0; trim_dim < chunk_shape.size(); ++trim_dim) {
				chunk_shape[trim_dim] = 1; //trim
				//is the new size less than the desired size?
				chunk_size = util::product(chunk_shape.begin(), chunk_shape.end(), size_t(1));
				if (chunk_size < desired_chunk_size) {
					//yes -- expand it as much as possible, then return
					chunk_shape[trim_dim] = desired_chunk_size / chunk_size;
					break;
				}
			}
			//the minimum chunk shape is a 1 in every dimension
			return chunked(chunk_shape);
		}
		DProps_& deflate(unsigned int level) {
			check(H5Pset_deflate(id, level));
			return *this;
		}
		DProps_& fletcher32() {
			check(H5Pset_fletcher32(id));
			return *this;
		}
		DProps_& shuffle() {
			check(H5Pset_shuffle(id));
			return *this;
		}
		bool is_chunked() const {
			return H5Pget_layout(id) == H5D_CHUNKED;
		}
		std::vector<hsize_t> chunk() const {
			int n = H5Pget_chunk(id, 0, nullptr);
			if (n <= 0)
				return std::vector<hsize_t>();
			std::vector<hsize_t> dims(n);
			H5Pget_chunk(id, n, dims.data());
			return dims;
		}
	};
	
	template<typename XX>
	const DProps DProps_<XX>::DEFAULT = DProps(H5P_DATASET_CREATE_DEFAULT);

	

	template<typename XX>
	class DSpace_ : public ID {
		friend class Dataset;
		friend class Attribute;
	protected:
		DSpace_(hid_t id) : ID(id) {}
	public:
		DSpace_() : ID(H5Screate(H5S_SCALAR)) {}
		//copy
		DSpace_(const DSpace_ &ds) : ID(H5Scopy(ds)) {}
		DSpace_& operator=(DSpace_ ds) {
			swap(ds);
			return *this;
		}
		//move
		DSpace_(DSpace_ &&ds) : ID(std::move(ds)) {}
		DSpace_& operator=(DSpace_&& ds) {
			steal(ds);
			return *this;
		}
		DSpace_(size_t N, const hsize_t *shape, const hsize_t *maxshape = nullptr) : ID(0) {
			if (N) {
				id = check_id(H5Screate_simple(int(N), shape, maxshape));
			}
			else {
				id = check_id(H5Screate(H5S_SCALAR));
			}
		}
		template<size_t N>
		explicit DSpace_(const hsize_t(&shape)[N]) : ID(0) {
			if (N) {
				id = check_id(H5Screate_simple(int(N), shape, nullptr));
			}
			else {
				id = check_id(H5Screate(H5S_SCALAR));
			}
		}
		template<size_t N>
		DSpace_(const hsize_t(&shape)[N], const hsize_t(&maxshape)[N]) : ID(0) {
			if (N) {
				id = check_id(H5Screate_simple(int(N), shape, maxshape));
			}
			else {
				id = check_id(H5Screate(H5S_SCALAR));
			}
		}
		explicit DSpace_(const std::vector<hsize_t>& shape) : ID(0) {
			if (shape.size()) {
				id = check_id(H5Screate(H5S_SIMPLE));
				extent(shape, shape);
			}
			else {
				id = check_id(H5Screate(H5S_SCALAR));
			}
		}
		DSpace_(const std::vector<hsize_t> &shape, const std::vector<hsize_t> &maxshape) : ID(0) {
			if (shape.size()) {
				id = check_id(H5Screate(H5S_SIMPLE));
				extent(shape, maxshape);
			}
			else {
				id = check_id(H5Screate(H5S_SCALAR));
			}
		}
		~DSpace_() {
			if (id) close();
		}
		void close() {
			check(H5Sclose(id));
		}
		DSpace_& select(const Selection& s) {
			s.set(*this);
			return *this;
		}
		void extent(const std::vector<hsize_t> &shape) {
			extent(shape, shape);
		}
		void extent(const std::vector<hsize_t> &shape, const std::vector<hsize_t> &maxshape) {
			check(H5Sset_extent_simple(id, int(shape.size()), shape.data(), maxshape.data()));
		}
		std::vector<hsize_t> extent() const {
			if (H5Sget_simple_extent_type(id) == H5S_SIMPLE) {
				int n = H5Sget_simple_extent_ndims(id);
				std::vector<hsize_t> shape(n);
				H5Sget_simple_extent_dims(id, shape.data(), nullptr);
				return shape;
			}
			else {
				return std::vector<hsize_t>();
			}
		}
		hssize_t count() const {
            return check_hssize(H5Sget_simple_extent_npoints(id));
        }
        hssize_t countSelected() const {
            return check_hssize(H5Sget_select_npoints(id));
        }
		std::vector<hsize_t> max_extent() const {
			if (H5Sget_simple_extent_type(id) == H5S_SIMPLE) {
				int n = H5Sget_simple_extent_ndims(id);
				std::vector<hsize_t> shape(n);
				H5Sget_simple_extent_dims(id, nullptr, shape.data());
				return shape;
			}
			else {
				return std::vector<hsize_t>();
			}
		}
		std::pair<std::vector<hsize_t>, std::vector<hsize_t>> extents() const {
			if (H5Sget_simple_extent_type(id) == H5S_SIMPLE) {
				int n = H5Sget_simple_extent_ndims(id);
				std::vector<hsize_t> shape(n), maxshape(n);
				H5Sget_simple_extent_dims(id, shape.data(), maxshape.data());
				return make_pair(shape, maxshape);
			}
			else {
				return make_pair(std::vector<hsize_t>(), std::vector<hsize_t>());
			}
		}
		bool extendable() const {
			if (H5Sget_simple_extent_type(id) == H5S_SIMPLE) {
				int n = H5Sget_simple_extent_ndims(id);
				std::vector<hsize_t> shape(n), maxshape(n);
				H5Sget_simple_extent_dims(id, shape.data(), maxshape.data());
				for (size_t i = 0; i < n; ++i) {
					if (maxshape[i] == H5S_UNLIMITED || shape[i] < maxshape[i])
						return true;
				}
			}
			return false;
		}
		static const size_t MAX_RANK;
		static const hsize_t UNLIMITED;
		static const hsize_t UNL;
		static const DSpace SCALAR;
	};
	template<typename XX> const size_t DSpace_<XX>::MAX_RANK = size_t(H5S_MAX_RANK);
	template<typename XX> const hsize_t DSpace_<XX>::UNLIMITED = hsize_t(H5S_UNLIMITED);
	template<typename XX> const hsize_t DSpace_<XX>::UNL = hsize_t(H5S_UNLIMITED);
	template<typename XX> const DSpace DSpace_<XX>::SCALAR = DSpace(H5Screate(H5S_SCALAR));

		template<typename XX>
	class Selection_ {
	public:
		virtual void set(DSpace& ds) const = 0;
		static const SelectAll ALL;
	};

	class SelectAll : public Selection {
		virtual void set(DSpace& ds) const {
			check(H5Sselect_all(ds));
		}
	};
	template<typename XX> const SelectAll Selection_<XX>::ALL;

	class Hyperslab : public Selection {
	protected:
		std::vector<hsize_t> start, stride, count, block;
	public:
		Hyperslab() {}
		Hyperslab(const Hyperslab& h) : Selection(h), start(h.start), stride(h.stride), count(h.count), block(h.block) {}
		Hyperslab(Hyperslab&& h) : Selection(std::move(h)), start(std::move(h.start)), stride(std::move(h.stride)), count(std::move(h.count)), block(std::move(h.block)) {}
		Hyperslab(const std::vector<hsize_t>& start, const std::vector<hsize_t>& count) : start(start), count(count) {}
		virtual void set(DSpace& ds) const {
			check(H5Sselect_hyperslab(ds, H5S_SELECT_SET, start.data(), stride.data(), count.data(), block.data()));
		}
	};

	template<typename data_t>
	DSpace space(const data_t& d) {
		return DSpace(H5TL::shape(d));
	}

	//Attributes
	class Attribute : public ID {
		friend class Object;
	protected:
		Attribute(hid_t id) : ID(id) {}
	public:
		Attribute() : ID(0) {}
		//no copy!
		//move
		Attribute(Attribute &&attr) : ID(std::move(attr)) {}
		Attribute& operator=(Attribute&& a) {
			steal(a);
			return *this;
		}
		virtual ~Attribute() {
			if (id) close();
		}
		virtual void close() {
			check(H5Aclose(id)); id = 0;
		}
		std::string name() {
			ssize_t len = check_ssize(H5Aget_name(id, 0, nullptr));
			std::string tmp(len, '\0');
			check_ssize(H5Aget_name(id, len, &(tmp[0])));
			return tmp;
		}
		DType dtype() {
			return DType(H5Aget_type(id));
		}
		DSpace space() {
			return DSpace(H5Aget_space(id));
		}
		void write(const void* buffer, const DType &buffer_type) {
			check(H5Awrite(id, buffer_type, buffer));
		}
		template<typename data_t>
		void write(const data_t& buffer) {
			write(H5TL::data(buffer), H5TL::dtype(buffer));
		}
		void read(void* buffer, const DType &buffer_type) {
			check(H5Aread(id, buffer_type, buffer));
		}
		template<typename data_t>
		void read(data_t& buffer) {
			read(H5TL::data(buffer), H5TL::dtype(buffer));
		}
		template<typename data_t>
		void read(data_t* buffer) {
			read(H5TL::data(buffer), H5TL::dtype(buffer));
		}
		template<typename data_t>
		void read() {
			auto buffer = H5TL::allocate<data_t>(space().extent(), dtype());
			read(buffer);
			return buffer;
		}
	};

	//Object
	//Files, Groups, Datasets
	class Object : public ID {
	protected:
		Object() : ID(0) {}
		Object(hid_t id) : ID(id) {}
	public:
		//no copy!
		//move
		Object(Object &&loc) : ID(std::move(loc)) {}
		Object& operator=(Object&& obj) {
			steal(obj);
			return *this;
		}
		virtual ~Object() {}

		virtual bool exists() {
			return check_tri(H5Oexists_by_name(id, ".", H5P_LINK_ACCESS_DEFAULT));
		}
		//attributes:
		bool hasAttribute(const std::string& name) {
			return check_tri(H5Aexists(id, name.c_str()));
		}
		Attribute createAttribute(const std::string& name, const DType& type, const DSpace& space) {
			return Attribute(H5Acreate(id, name.c_str(), type, space, H5P_ATTRIBUTE_CREATE_DEFAULT, H5P_DEFAULT));
		}
		Attribute attribute(const std::string& name) {
			return Attribute(H5Aopen(id, name.c_str(), H5P_DEFAULT));
		}
		Attribute writeAttribute(const std::string& name, const void* buffer, const DType& buffer_type, const DSpace& space) {
			Attribute tmp = createAttribute(name, buffer_type, space);
			tmp.write(buffer, buffer_type);
			return tmp;
		}
		template<typename data_t>
		Attribute writeAttribute(const std::string& name, const data_t& buffer, const DSpace& space) {
			return writeAttribute(name, H5TL::data(buffer), H5TL::dtype(buffer), space);
		}
		template<typename data_t>
		Attribute writeAttribute(const std::string& name, const data_t& buffer) {
			return writeAttribute(name, H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer));
		}
		Attribute readAttribute(const std::string& name, void* buffer, const DType& buffer_type) {
			Attribute tmp = attribute(name);
			tmp.read(buffer, buffer_type);
			return tmp;
		}
		template<typename data_t>
		Attribute readAttribute(const std::string& name, data_t& buffer) {
			return readAttribute(name, H5TL::data(buffer), H5TL::dtype(buffer));
		}
	};

	class Dataset : public Object {
		friend class Group;
	protected:
		Dataset(hid_t id) : Object(id) {}
	public:
		Dataset() : Object() {}
		//no copy!
		//move
		Dataset(Dataset &&dset) : Object(std::move(dset)) {}
		Dataset& operator=(Dataset&& dset) {
			steal(dset);
			return *this;
		}
		virtual ~Dataset() {
			if (id) close();
		}
		virtual void close() {
			check(H5Dclose(id)); id = 0;
		}
		DSpace space() {
			return DSpace(H5Dget_space(id));
		}
		DType dtype() {
			return DType(H5Dget_type(id));
		}
		//write
		void write(const void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			check(H5Dwrite(id, buffer_type, buffer_shape, space().select(selection), H5P_DATASET_XFER_DEFAULT, buffer));
		}
		void write(const void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			//we need to pad buffer_shape and offset to the correct dimensionality for making the selection
			//get the current extent of the dataset:
			auto extent = space().extent();
			//get the extent of the buffer and prepend with 1's:
			auto buffer_extent = buffer_shape.extent();
			if (buffer_extent.size() < extent.size())
				util::prepend(buffer_extent, extent.size() - buffer_extent.size(), hsize_t(1));
			//prepend the offset with 0s:
			std::vector<hsize_t> new_offset(offset);
			if (new_offset.size() < extent.size())
				util::prepend(new_offset, extent.size() - new_offset.size(), hsize_t(0));
			write(buffer, buffer_type, buffer_shape, Hyperslab(new_offset, buffer_extent));
		}
		template<typename data_t>
		void write(const data_t& buffer, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			write(H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape);
		}
		template<typename data_t>
		void write(const data_t& buffer, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			write(H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape, offset);
		}
		template<typename data_t>
		void write(const data_t& buffer, const DType& buffer_type, const Selection& selection = Selection::ALL) {
			write(H5TL::data(buffer), buffer_type, H5TL::space(buffer), selection);
		}
		template<typename data_t>
		void write(const data_t& buffer, const DType& buffer_type, const std::vector<hsize_t>& offset) {
			write(H5TL::data(buffer), buffer_type, H5TL::space(buffer), offset);
		}
		template<typename data_t>
		void write(const data_t& buffer, const Selection& selection = Selection::ALL) {
			write(H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer));
		}
		template<typename data_t>
		void write(const data_t& buffer, const std::vector<hsize_t>& offset) {
			write(H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer), offset);
		}
		//extend
		void extend(const hsize_t* extent) {
			check(H5Dextend(id, extent));
		}
		void extend(const std::vector<hsize_t>& extent) {
			extend(extent.data());
		}
		//append with offset -- like write with offset, but checks to see if the dataset needs to be extended first
		void append(const void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			//get the current and maximum extents of the dataset
			std::vector<hsize_t> current_extent, max_extent;
			std::tie(current_extent, max_extent) = space().extents();
			//get the buffer extents, prepend 1s as necessary to make it the correct size
			std::vector<hsize_t> buffer_extent = buffer_shape.extent();
			if (buffer_extent.size() < current_extent.size()) {
				util::prepend(buffer_extent, current_extent.size() - buffer_extent.size(), hsize_t(1));
			}
			//do we need to extend the dataset?
			//the new extents will be the offset + buffer_extent
			std::vector<hsize_t> new_extent(offset.size());
			std::transform(buffer_extent.begin(), buffer_extent.end(), offset.begin(), new_extent.begin(), std::plus<hsize_t>());
			//if any element of the new extents are > current extents, we need to extend the dataset
			if (!std::equal(new_extent.begin(), new_extent.end(), current_extent.begin(), std::less_equal<hsize_t>())) {
				extend(new_extent);
			}
			//now that the dataset is extended, we can write into it
			write(buffer, buffer_type, buffer_shape, offset);
		}
		template<typename data_t>
		void append(const data_t& buffer, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			append(H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape, offset);
		}
		template<typename data_t>
		void append(const data_t& buffer, const DType& buffer_type, const std::vector<hsize_t>& offset) {
			append(H5TL::data(buffer), buffer_type, H5TL::space(buffer), offset);
		}
		template<typename data_t>
		void append(const data_t& buffer, const std::vector<hsize_t>& offset) {
			append(H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer), offset);
		}
		//append without offset -- automatically extends the slowest varying dimension (0)
		void append(const void *buffer, const DType& buffer_type, const DSpace& buffer_shape) {
			//get the current extent
			std::vector<hsize_t> current_extent = space().extent();
			//get the buffer shape, and extend as necessary
			std::vector<hsize_t> buffer_extent = buffer_shape.extent();
			if (buffer_extent.size() < current_extent.size()) {
				util::prepend(buffer_extent, current_extent.size() - buffer_extent.size(), hsize_t(1));
			}
			//we only extend the slowest varying dimension for now
			//TODO: extend the fastest varying extendable dimension?
			size_t dim_to_extend = 0;
			//compute the offset into the file where we will store the data
			std::vector<hsize_t> offset(current_extent.size(), 0);
			offset[dim_to_extend] = current_extent[dim_to_extend];
			current_extent[dim_to_extend] += buffer_extent[dim_to_extend];
			//extend
			extend(current_extent);
			//write
			write(buffer, buffer_type, DSpace(buffer_extent), offset);
		}

		template<typename data_t>
		void append(const data_t& buffer, const DSpace& buffer_shape) {
			append(H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape);
		}
		template<typename data_t>
		void append(const data_t& buffer, const DType& buffer_type) {
			append(H5TL::data(buffer), buffer_type, H5TL::space(buffer));
		}
		template<typename data_t>
		void append(const data_t& buffer) {
			append(H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer));
		}
		//read
		void read(void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			//if buffer_shape is empty, allocate space to hold the selection???
			check(H5Dread(id, buffer_type, buffer_shape, space().select(selection), H5P_DATASET_XFER_DEFAULT, buffer));
		}
		void read(void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			read(buffer, buffer_type, buffer_shape, Hyperslab(offset, buffer_shape.extent()));
		}
		//read w/ reference to buffer
		template<typename data_t>
		void read(data_t& buffer, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			read(H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape, selection);
		}
		template<typename data_t>
		void read(data_t& buffer, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			read(H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape, offset);
		}
		template<typename data_t>
		void read(data_t& buffer, const Selection& selection = Selection::ALL) {
			read(H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer), selection);
		}
		template<typename data_t>
		void read(data_t& buffer, const std::vector<hsize_t>& offset) {
			read(H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer), offset);
		}
		//read w/ pointer to buffer
		template<typename data_t>
		void read(data_t* buffer, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			read(H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape, selection);
		}
		template<typename data_t>
		void read(data_t* buffer, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			read(H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape, offset);
		}
		template<typename data_t>
		void read(data_t* buffer, const Selection& selection = Selection::ALL) {
			read(H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer), selection);
		}
		template<typename data_t>
		void read(data_t* buffer, const std::vector<hsize_t>& offset) {
			read(H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer), offset);
		}
		//read with allocate
		template<typename data_t>
		typename adapt<data_t>::allocate_return
			read(const std::vector<hsize_t>& offset, const std::vector<hsize_t>& buffer_shape) {
				auto buffer = H5TL::allocate<data_t>(buffer_shape, dtype());
				read(buffer, offset);
				return buffer;
		}
		template<typename data_t>
		typename adapt<data_t>::allocate_return
			read() {
				auto buffer = H5TL::allocate<data_t>(space().extent(), dtype());
				read(buffer);
				return buffer;
		}
	};

	//Files, Groups
	class Group : public Object {
	protected:
		Group(hid_t id) : Object(id) {}
	public:
		Group() : Object() {}
		//no copy!
		//move:
		Group(Group &&grp) : Object(std::move(grp)) {}
		Group& operator=(Group&& grp) {
			steal(grp);
			return *this;
		}
		virtual ~Group() {
			if (id) close();
		}
		virtual void close() {
			check(H5Gclose(id));
		}
		using ID::valid;
		virtual bool valid(const std::string &path) {
			//Copy logic of H5TLpath_valid
			//the path to the current object or root is always valid:
			if (path == "." || path == "/" || path == "./")
				return true;

			//make a working copy of the path
			std::string current_path(path);
			//we want to check each step along the path, except the last
			//so find each path delimiter in turn
			size_t delimiter_pos = 0;
			if (path.compare(0, 1, "/") == 0)
				delimiter_pos = 1; //skip the first '/' of an absolute path
			else if (path.compare(0, 2, "./") == 0)
				delimiter_pos = 2; //the path starts with "./" -- skip this part
			while ((delimiter_pos = path.find('/', delimiter_pos + 1)) != std::string::npos) {
				//if the path ends with a '/', we should ignore it in this loop
				if (delimiter_pos == path.size() - 1) break;
				//change the character to null
				current_path[delimiter_pos] = '\0';
				//check the link
				if (!check_tri(H5Lexists(id, current_path.c_str(), H5P_LINK_ACCESS_DEFAULT))) return false;
				//check that the link resolves to an object
				if (!check_tri(H5Oexists_by_name(id, current_path.c_str(), H5P_LINK_ACCESS_DEFAULT))) return false;
				//replace the delimiter and move on to the next step of the path
				current_path[delimiter_pos] = '/';
			}
			//all of the steps of the path have been checked, except the last
			//make sure the link to the last object exists
			return check_tri(H5Lexists(id, current_path.c_str(), H5P_LINK_ACCESS_DEFAULT));
		}
		virtual bool exists(const std::string &path) {
			if (!valid(path))
				return false;
			return check_tri(H5Oexists_by_name(id, path.c_str(), H5P_LINK_ACCESS_DEFAULT));
		}
		bool hasAttribute(const std::string& object, const std::string& attr) {
			return check_tri(H5Aexists_by_name(id, object.c_str(), attr.c_str(), H5P_LINK_ACCESS_DEFAULT));
		}

		Group group(const std::string &name) {
			return Group(H5Gopen(id, name.c_str(), H5P_GROUP_ACCESS_DEFAULT));
		}
		Group createGroup(const std::string &name) {
			return Group(H5Gcreate(id, name.c_str(), LProps::DEFAULT, H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT));
		}
		Dataset dataset(const std::string &name) {
			return Dataset(H5Dopen(id, name.c_str(), H5P_DATASET_ACCESS_DEFAULT));
		}
		Dataset createDataset(const std::string &name, const DType &dt, const DSpace &space, const DProps& props = DProps::DEFAULT) {
			//make a local copy of the dataset creation properties:
			DProps _props(props);
			//if space is extendable, but not chunked, we need to chunk it
			if (!_props.is_chunked() && space.extendable())
				_props.chunked();
			//if props is chunked, but does not have chunk dimensions yet, we need to compute them
			if (_props.is_chunked() && _props.chunk().size() == 0) {
				_props.chunked(space.extent(), dt.size());
				return Dataset(H5Dcreate(id, name.c_str(), dt, space, LProps::DEFAULT, _props, H5P_DATASET_ACCESS_DEFAULT));
			}
			else {
				//no changes necessary
				return Dataset(H5Dcreate(id, name.c_str(), dt, space, LProps::DEFAULT, _props, H5P_DATASET_ACCESS_DEFAULT));
			}
		}
		//create dataset and write data in
		Dataset write(const std::string &name, const void* buffer, const DType &dt, const DSpace &space, const DProps& props = DProps::DEFAULT) {
			//create and write in one fell swoop
			Dataset dset = createDataset(name, dt, space, props);
			dset.write(buffer, dt, space);
			return dset;
		}
		template<typename data_t>
		Dataset write(const std::string &name, const data_t& buffer, const DSpace &space, const DProps &props = DProps::DEFAULT) {
			return write(name, H5TL::data(buffer), H5TL::dtype(buffer), space, props);
		}
		template<typename data_t>
		Dataset write(const std::string &name, const data_t& buffer, const DProps &props = DProps::DEFAULT) {
			return write(name, H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer), props);
		}
		//open and read dataset
		Dataset read(const std::string &name, void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			Dataset ds = dataset(name);
			ds.read(buffer, buffer_type, buffer_shape, selection);
			return ds;
		}
		Dataset read(const std::string &name, void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			return read(name, buffer, buffer_type, buffer_shape, Hyperslab(offset, buffer_shape.extent()));
		}
		//read with reference to buffer
		template<typename data_t>
		Dataset read(const std::string &name, data_t& buffer, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			return read(name, H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape, selection);
		}
		template<typename data_t>
		Dataset read(const std::string &name, data_t& buffer, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			return read(name, H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape, offset);
		}
		template<typename data_t>
		Dataset read(const std::string &name, data_t& buffer, const Selection& selection = Selection::ALL) {
			return read(name, H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer), selection);
		}
		template<typename data_t>
		Dataset read(const std::string &name, data_t& buffer, const std::vector<hsize_t>& offset) {
			return read(name, H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer), offset);
		}
		//read with pointer to buffer
		template<typename data_t>
		Dataset read(const std::string &name, data_t* buffer, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			return read(name, H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape, selection);
		}
		template<typename data_t>
		Dataset read(const std::string &name, data_t* buffer, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			return read(name, H5TL::data(buffer), H5TL::dtype(buffer), buffer_shape, offset);
		}
		template<typename data_t>
		Dataset read(const std::string &name, data_t* buffer, const Selection& selection = Selection::ALL) {
			return read(name, H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer), selection);
		}
		template<typename data_t>
		Dataset read(const std::string &name, data_t* buffer, const std::vector<hsize_t>& offset) {
			return read(name, H5TL::data(buffer), H5TL::dtype(buffer), H5TL::space(buffer), offset);
		}
		//open dataset, read with allocate
		template<typename data_t>
		typename adapt<data_t>::allocate_return
			read(const std::string &name, const std::vector<hsize_t>& offset, const std::vector<hsize_t>& buffer_shape) {
				Dataset ds = dataset(name);
				return ds.read<data_t>(offset, buffer_shape);
		}
		template<typename data_t>
		typename adapt<data_t>::allocate_return
			read(const std::string &name) {
				Dataset ds = dataset(name);
				return ds.read<data_t>();
		}
		//linking
		void createHardLink(const std::string& name, const Group& target_group, const std::string& target) {
			check(H5Lcreate_hard(target_group, target.c_str(), id, name.c_str(), LProps::DEFAULT, H5P_LINK_ACCESS_DEFAULT));
		}
		void createHardLink(const std::string& name, const std::string& target) {
			createHardLink(name, *this, target);
		}
		void createHardLink(const std::string& name, const Object& target) {
			check(H5Olink(target, id, name.c_str(), LProps::DEFAULT, H5P_LINK_ACCESS_DEFAULT));
		}
		void createLink(const std::string& name, const std::string& target) {
			check(H5Lcreate_soft(target.c_str(), id, name.c_str(), LProps::DEFAULT, H5P_LINK_ACCESS_DEFAULT));
		}
		void createLink(const std::string& name, const std::string& file, const std::string& target) {
			check(H5Lcreate_external(file.c_str(), target.c_str(), id, name.c_str(), LProps::DEFAULT, H5P_LINK_ACCESS_DEFAULT));
		}
	};

	//Files
	class File : public Group {
	public:
		enum OpenMode : unsigned int {
			#pragma push_macro("H5CHECK") //compatible with MSVC, gcc, clang
			#undef H5CHECK
			#define H5CHECK //need to make this not call H5check() so that the following expressions are constant:
			TRUNCATE = H5F_ACC_TRUNC,
			CREATE = H5F_ACC_EXCL,
			READ_WRITE = H5F_ACC_RDWR,
			READ = H5F_ACC_RDONLY
			#pragma pop_macro("H5CHECK")
		};
		File(const std::string& name, const OpenMode& mode = READ_WRITE) {
			open(name, mode);
		}
		File() {}
		//no copy!
		//move
		File(File&& f) : Group(std::move(f)) {}
		File& operator=(File&& f) {
			steal(f);
			return *this;
		}
		virtual ~File() {
			if (id) close();
		}
		virtual void open(const std::string& name, const OpenMode& mode = READ_WRITE) {
			if (id) close();
			if (mode == TRUNCATE || mode == CREATE) {
				id = check_id(H5Fcreate(name.c_str(), mode, H5P_FILE_CREATE_DEFAULT, H5P_FILE_ACCESS_DEFAULT));
			}
			else {
				hid_t tmp_id = H5Fopen(name.c_str(), mode, H5P_FILE_ACCESS_DEFAULT);
				if (tmp_id < 0) //if we failed to open, let's try creating the file
					tmp_id = H5Fcreate(name.c_str(), CREATE, H5P_FILE_CREATE_DEFAULT, H5P_FILE_ACCESS_DEFAULT);
				id = check_id(tmp_id);
			}
		}
		virtual void close() {
			check(H5Fclose(id)); id = 0;
		}
	};
}

/*****************\
* Type Adapters *
\*****************/

/*	Here is the required struct for compatibility.
Use the "enable" type parameter with std::enable_if to provide specializations for a whole class of types
- see arithmetic type adapter for example
- N.B. that all such specializations must be disjoint, or there will be ambiguities for the compiler (e.g. since there's an is_arithmetic specialization, we can't also have an is_integral specialization)
For partial compatibility, use static_assert(false, "message") to provide nice error messages. See the pointer-type adapter for examples

template<typename data_t, typename enable=void>
struct adapt {
typedef const DType& dtype_return;
typedef void* data_return;
typedef const void* const_data_return;
typedef data_t allocate_return;

static size_t rank(const data_t&);
static std::vector<hsize_t> shape(const data_t&);
static dtype_return dtype(const data_t&);
static data_return data(data_t&);
static const_data_return data(const data_t&);
static allocate_return allocate(const std::vector<hsize_t>&, const DType&);
};
*/
namespace H5TL {

	//typedef the int type with the given # of bytes
	template<std::size_t sz> struct intx_t; //undefined
	template<> struct intx_t<1> { typedef int8_t type; };
	template<> struct intx_t<2> { typedef int16_t type; };
	template<> struct intx_t<4> { typedef int32_t type; };
	template<> struct intx_t<8> { typedef int64_t type; };

	typedef intx_t<sizeof(bool)>::type intbool_t;

	//convert bool types to int types, leave others unchanged
	template<typename T>
	struct bool_to_int { typedef T type; };
	template<> struct bool_to_int<bool> { typedef intbool_t type; };
	template<> struct bool_to_int<const bool> { typedef const intbool_t type; };


	//arithmetic & bool type adapter
	template<typename number_t>
	struct adapt<number_t, typename std::enable_if<std::is_arithmetic<typename bool_to_int<number_t>::type>::value>::type> {
		typedef typename std::remove_cv<number_t>::type data_t;
		typedef typename bool_to_int<data_t>::type data_nbt; //no bool type
		typedef const DType& dtype_return;
		typedef data_nbt* data_return;
		typedef const data_nbt* const_data_return;
		typedef data_t allocate_return;

		static size_t rank(const data_t&) {
			return 0;
		}
		static std::vector<hsize_t> shape(const data_t&) {
			return std::vector<hsize_t>();
		}
		static dtype_return dtype(const data_t&) {
			return H5TL::dtype<data_nbt>();
		}
		static data_return data(data_t& d) {
			return (data_return)(&d);
		}
		static const_data_return data(const data_t& d) {
			return (const_data_return)(&d);
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			size_t n = util::product(shape.begin(), shape.end(), hsize_t(1));
			if (n != 1)
				throw std::runtime_error("Cannot allocate data_t with shape = {" + util::join(", ", shape.begin(), shape.end()) + "}"); //TODO: convert the shape to a string
			return data_t(0);
		}
	};

	//constant sized array adapter
	template<typename T, size_t N>
	struct adapt<T[N]> {
		typedef typename std::remove_cv<T>::type data_t;
		typedef typename bool_to_int<data_t>::type data_nbt;
		typedef const DType& dtype_return;
		typedef data_nbt* data_return;
		typedef const data_nbt* const_data_return;
		typedef data_t* allocate_return;

		static size_t rank(const T(&)[N]) {
			return 1;
		}
		static std::vector<hsize_t> shape(const data_t(&)[N]) {
			return std::vector<hsize_t>(1, N);
		}
		static dtype_return dtype(const data_t(&)[N]) {
			return H5TL::dtype<data_nbt>();
		}
		static data_return data(data_t(&d)[N]) {
			return (data_return)std::begin(d);
		}
		static const_data_return data(const data_t(&d)[N]) {
			return (data_return)std::begin(d);
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			if (util::product(begin(shape), end(shape), hsize_t(1)) != N)
				throw std::runtime_error("Cannot allocate fixed sized array with conflicting shape = {" + util::join(", ", shape.begin(), shape.end()) + "}");
			return new data_t[N];
		}
	};

	//C string adapter
	template<size_t N>
	struct adapt<const char[N]> {
		typedef const char data_t;
		typedef DType dtype_return;
		typedef const char* const_data_return;
		typedef data_t* allocate_return;

		static size_t rank(const data_t(&)[N]) {
			return 0;
		}
		static std::vector<hsize_t> shape(const data_t(&)[N]) {
			return std::vector<hsize_t>();
		}
		static dtype_return dtype(const data_t(&)[N]) {
			return DType(DType::STRING, N);
		}
		static const_data_return data(const data_t(&d)[N]) {
			return std::begin(d);
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			static_assert(false, "Cannot allocate a C-style string. Please use std::string instead");
		}
	};

	//pointer adapter
	template<typename cvptr_t>
	struct adapt<cvptr_t, typename std::enable_if<std::is_pointer<cvptr_t>::value
		&& !std::is_void<typename std::remove_pointer<cvptr_t>::type>::value
	>::type> {
		typedef typename std::remove_cv<cvptr_t>::type ptr_t;
		typedef typename std::remove_cv<typename std::remove_pointer<ptr_t>::type>::type data_t;
		typedef typename bool_to_int<data_t>::type data_nbt;
		typedef const DType& dtype_return;
		typedef data_nbt* data_return;
		typedef const data_nbt const_data_return;
		typedef data_t* allocate_return;

		static size_t rank(const ptr_t&) {
			static_assert(false, "Cannot determine rank of data from pointer");
		}
		static std::vector<hsize_t> shape(const ptr_t&) {
			static_assert(false, "Cannot determine shape of data from pointer");
		}
		static dtype_return dtype(const ptr_t&) {
			return H5TL::dtype<data_nbt>();
		}
		static data_return data(data_t*& p) {
			return p;
		}
		static const_data_return data(const data_t*& p) {
			return p;
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			size_t n = util::product(shape.begin(), shape.end(), hsize_t(1));
			return new data_t[n];
		}
	};
	//void pointer adapter
	template<>
	struct adapt<void*> {
		typedef const DType& dtype_return;
		typedef void* data_return;
		typedef const void* const_data_return;
		typedef void* allocate_return;

		static size_t rank(const void*);
		static std::vector<hsize_t> shape(const void*);
		static dtype_return dtype(const void*);
		static data_return data(void* d) {
			return d;
		}
		static const_data_return data(const void* d) {
			return d;
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType& dt) {
			size_t n = util::product(shape.begin(), shape.end(), hsize_t(dt.size()));
			return ::operator new[](n);
		}
	};
}


#define H5TL_STD_ADAPT
#ifdef H5TL_STD_ADAPT
//Shape, rank, dtype, data functions for standard container types:
namespace H5TL {

	template<>
	struct adapt<std::string> {
		typedef DType dtype_return;
		typedef std::string::pointer data_return;
		typedef std::string::const_pointer const_data_return;
		typedef std::string allocate_return;

		static size_t rank(const std::string&) {
			return 0;
		}
		static std::vector<hsize_t> shape(const std::string&) {
			return std::vector<hsize_t>();
		}
		static dtype_return dtype(const std::string& s) {
			return DType(DType::STRING, s.size());
		}
		static data_return data(std::string& s) {
			return &(s[0]); //s.data() always returns const pointer :(
		}
		static const_data_return data(const std::string& s) {
			return s.data();
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			if (shape.size() > 1 && util::product(shape.begin() + 1, shape.end(), hsize_t(1)) > 1) //TODO: does this make sense?
				throw std::runtime_error("Cannot allocate std::string with rank > 1");
			return std::string(shape[0], '\0');
		}
	};

	//std::array
	template<typename T, size_t N>
	struct adapt<std::array<T, N>, void> {
		typedef T data_t;
		typedef typename bool_to_int<data_t>::type data_nbt;
		typedef const DType& dtype_return;
		typedef data_nbt* data_return;
		typedef const data_nbt* const_data_return;
		typedef std::array<data_t, N> allocate_return;

		static size_t rank(const std::array<T, N>&) {
			return 1;
		}
		static std::vector<hsize_t> shape(const std::array<T, N>&) {
			return std::vector<hsize_t>(1, hsize_t(N));
		}
		static dtype_return dtype(const std::array<T, N>&) {
			return H5TL::dtype<data_nbt>();
		}
		static data_return data(std::array<T, N> &d) {
			return (data_return)d.data();
		}
		static const_data_return data(const std::array<T, N>& d) {
			return (const_data_return)d.data();
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			if (util::product(shape.begin(), shape.end(), hsize_t(1)) != N)
				throw std::runtime_error("Cannot allocate std::array<T,N> with shape = {" + util::join(", ", shape.begin(), shape.end()) + "}.");
			return std::array<T, N>();
		}
	};

	//std::vector
	template<typename T>
	struct adapt<std::vector<T>> {
		typedef T data_t;
		typedef typename bool_to_int<data_t>::type data_nbt;
		typedef const DType& dtype_return;
		typedef data_nbt* data_return;
		typedef const data_nbt* const_data_return;
		typedef std::vector<T> allocate_return;

		static size_t rank(const std::vector<T>&) {
			return 1;
		}
		static std::vector<hsize_t> shape(const std::vector<T>& v) {
			return std::vector<hsize_t>(1, v.size());
		}
		static dtype_return dtype(const std::vector<T>&) {
			return H5TL::dtype<data_nbt>();
		}
		static data_return data(std::vector<T>& v) {
			return (data_return)v.data();
		}
		static const_data_return data(const std::vector<T>& v) {
			return (const_data_return)v.data();
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			return std::vector<T>(util::product(shape.begin(), shape.end(), hsize_t(1)), T());
		}
	};
}
#endif

#ifdef H5TL_BLITZ_ADAPT
//BLITZ++ shape, rank, dtype, data adapters
#include "blitz/array.h"
namespace H5TL {
	template<typename T, int N>
	struct adapt<blitz::Array<T, N>> {
		typedef T data_t;
		typedef typename bool_to_int<data_t>::type data_nbt;
		typedef const DType& dtype_return;

		typedef data_nbt* data_return;
		typedef const data_nbt* const_data_return;
		typedef blitz::Array<T, N> allocate_return;

		static size_t rank(const blitz::Array<T, N>&) {
			return N;
		}
		static std::vector<hsize_t> shape(const blitz::Array<T, N>& d) {
			const blitz::TinyVector<int, N> &s = d.shape();
			return std::vector<hsize_t>(s.begin(), s.end());
		}
		static dtype_return dtype(const blitz::Array<T, N>&) {
			return H5TL::dtype<data_nbt>();
		}
		static data_return data(blitz::Array<T, N>& d) {
			return (data_return)d.data();
		}
		static const_data_return data(const blitz::Array<T, N>& d) {
			return (const_data_return)d.data();
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			if (shape.size() > N)
				throw std::runtime_error("Cannot allocate blitz::Array<T,N> with higher dimensionality shape = {" + util::join(", ", shape.begin(), shape.end()) + "}.");
			blitz::TinyVector<int, N> extent(1);
			//we can't use std::copy_backward on MSVC because blitz::TinyVector::iterator is just a raw pointer
			auto in = shape.end();
			auto out = extent.end();
			while (in != shape.begin())
				*(--out) = int(*(--in));
			return blitz::Array<T, N>(extent);
		}
	};
}
#endif

//#define H5TL_OCV_ADAPT
#ifdef H5TL_OCV_ADAPT
//OpenCV shape, rank, dtype, data adapters
#include "opencv2/opencv.hpp"

namespace H5TL {
	template<>
	struct adapt<cv::Mat> {
	protected:
		static const DType& cv_h5(int cv_type) {
			switch (CV_MAT_DEPTH(cv_type)) {
			case CV_8U:
				return DType::UINT8;
			case CV_8S:
				return DType::INT8;
			case CV_16U:
				return DType::UINT16;
			case CV_16S:
				return DType::INT16;
			case CV_32S:
				return DType::INT32;
			case CV_32F:
				return DType::FLOAT;
			case CV_64F:
				return DType::DOUBLE;
			default:
				throw std::runtime_error("Datatype is not convertible from cv::Mat to H5TL::DType");
			}
		}
		static int h5_cv(const DType& dt) {
			if (dt == DType::UINT8)
				return CV_8U;
			else if (dt == DType::INT8)
				return CV_8S;
			else if (dt == DType::UINT16)
				return CV_16U;
			else if (dt == DType::INT16)
				return CV_16S;
			else if (dt == DType::INT32)
				return CV_32S;
			else if (dt == DType::FLOAT)
				return CV_32F;
			else if (dt == DType::DOUBLE)
				return CV_64F;
			else
				throw std::runtime_error("Datatype is not convertible from H5TL::DType to cv::Mat type");
		}
	public:
		typedef const DType& dtype_return;
		typedef void* data_return;
		typedef const void* const_data_return;
		typedef cv::Mat allocate_return;

		static size_t rank(const cv::Mat& d) {
			return d.dims + (d.channels() > 1 ? 1 : 0);
		}
		static std::vector<hsize_t> shape(const cv::Mat& d) {
			std::vector<hsize_t> tmp(rank(d));
			std::copy(d.size.p, d.size.p + d.dims, tmp.begin());
			if (d.channels() > 1)
				tmp[d.dims] = d.channels();
			return tmp;
		}
		static dtype_return dtype(const cv::Mat& d) {
			return cv_h5(d.type());
		}
		static data_return data(cv::Mat& d) {
			return d.data;
		}
		static const_data_return data(const cv::Mat& d) {
			return d.data;
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType& dt) {
			//allocate and then fill using std::transform with explicit cast -- avoids conversion warnings blowing up the console
			std::vector<int> sz(shape.size());
			std::transform(shape.begin(), shape.end(), sz.begin(), [](hsize_t x){return int(x); });
			return cv::Mat(int(sz.size()), sz.data(), h5_cv(dt));
		}
	};
	/*
	template<typename T>
	struct adapt<cv::Mat_<T>> {
	typedef const DType& dtype_return;
	typedef void* data_return;
	typedef const void* const_data_return;
	typedef data_t allocate_return;

	static size_t rank(const data_t&);
	static std::vector<hsize_t> shape(const data_t&);
	static dtype_return dtype(const data_t&);
	static data_return data(data_t&);
	static const_data_return data(const data_t&);
	static allocate_return allocate(const std::vector<hsize_t>&);
	}; */
}
#endif


#endif // H5TL_HPP
