#pragma once

#include "hdf5.h"

#include <utility>
#include <vector>
#include <array>
#include <iterator>
#include <numeric>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace std {
	template<typename InputIt, typename T>
	T product(InputIt first, InputIt last, T init) {
		return accumulate(first,last,init,multiplies<T>());
	}

	template<typename T>
	T clip(const T& val, const T& min, const T& max) {
		if(val < min) return min;
		else if(val > max) return max;
		else return val;
	}

	template<typename InputIt>
	string join(const std::string& delim, InputIt first, InputIt last) {
		ostringstream builder;
		builder << *first; ++first;
		while(first != last) {
			builder << delim << *first;
			++first;
		}
		return builder.str();
	}
}



namespace H5TL {
	//General library interface:

	/**
	Initializes the HDF5 library.
	*/
	inline void open() {
		if(H5open() < 0) throw std::runtime_error("open failed.");
	}
	/**
	Close the HDF5 library.
	*/
	inline void close() {
		if(H5close() < 0) throw std::runtime_error("close failed.");
	}
	
	class ID {
	protected:
		hid_t id;
		ID() : id(0) {}
		ID(hid_t _id) : id(_id) {}
	public:
		ID(const ID& i) : id(i.id) {}
		ID(ID &&i) : id(i.id) {i.id=0;}
		virtual ~ID() {}
		virtual void close()=0;
		virtual operator hid_t() const {return id;}
	};

	class DType;
	class PDType;

	class DType : public ID {
	protected:
		DType(hid_t id) : ID(id) {}
		DType(hid_t id, size_t sz) : ID(H5Tcopy(id)) {
			size(sz);
		}
	public:
		DType(const DType &dt) : ID(H5Tcopy(dt)) {}
		DType(DType &&dt) : ID(std::move(dt)) {}
		DType(const DType &dt, size_t sz) : ID(H5Tcopy(dt)) {
			size(sz);
		}
		virtual ~DType() {
			if(id) close();
		}
		void size(size_t sz) {
			if(sz == 0) H5Tset_size(id,H5T_VARIABLE);
			else H5Tset_size(id,sz);
		}
		virtual size_t size() const {
			return H5Tget_size(id);
		}
		void close() {
			H5Tclose(id); id = 0;
		}
	static const PDType INT8;
	static const PDType UINT8;
	static const PDType INT16;
	static const PDType UINT16;
	static const PDType INT32;
	static const PDType UINT32;
	static const PDType INT64;
	static const PDType UINT64;
	static const PDType FLOAT;
	static const PDType DOUBLE;
	static const DType STRING;
	};
	class PDType : public DType {
		friend class DType;
	protected:
		PDType(hid_t id) : DType(id) {}
	public:
		PDType(const PDType& dt) : DType(dt.id) {}
		PDType(PDType &&dt) : DType(std::move(dt)) {}
		virtual ~PDType() {id = 0;} //doesn't need to close(), but set id=0 so other destructors don't try to close it!
		void close() {}
	};
	const PDType DType::INT8(H5T_NATIVE_INT8);
	const PDType DType::UINT8(H5T_NATIVE_UINT8);
	const PDType DType::INT16(H5T_NATIVE_INT16);
	const PDType DType::UINT16(H5T_NATIVE_UINT16);
	const PDType DType::INT32(H5T_NATIVE_INT32);
	const PDType DType::UINT32(H5T_NATIVE_UINT32);
	const PDType DType::INT64(H5T_NATIVE_INT64);
	const PDType DType::UINT64(H5T_NATIVE_UINT64);
	const PDType DType::FLOAT(H5T_NATIVE_FLOAT);
	const PDType DType::DOUBLE(H5T_NATIVE_DOUBLE);
	const DType DType::STRING(H5T_C_S1, 0);

	//template function returns reference to predefined datatype
	template<typename T> const DType& dtype() { static_assert(false,"Type T is not convertible to a H5TL::DType");}
	//specialization for basic types:
	template<> const DType& dtype<int8_t>() {return DType::INT8;}
	template<> const DType& dtype<uint8_t>() {return DType::UINT8;}
	template<> const DType& dtype<int16_t>() {return DType::INT16;}
	template<> const DType& dtype<uint16_t>() {return DType::UINT16;}
	template<> const DType& dtype<int32_t>() {return DType::INT32;}
	template<> const DType& dtype<uint32_t>() {return DType::UINT32;}
	template<> const DType& dtype<int64_t>() {return DType::INT64;}
	template<> const DType& dtype<uint64_t>() {return DType::UINT64;}
	template<> const DType& dtype<float>() {return DType::FLOAT;}
	template<> const DType& dtype<double>() {return DType::DOUBLE;}
	template<> const DType& dtype<std::string>() {return DType::STRING;}
	//const char* might be ambiguous, so leaving it out for now

	//H5TL adapter traits, enable parameter is for using enable_if for specializations
	//specializations for compatible types are at the end of this file
	template<typename data_t, typename enable=void>
	struct adapt {
		//default implementation does not compile
		static_assert(false,"No specialization of H5TL::adapt for data_t. Please enable, include, or write an appropriate adapt specialization.");
		
		//here are required typedefs and member functions for complete compatibility
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
	allocate(const std::vector<hsize_t>& shape) {
		return adapt<data_t>::allocate(shape);
	}
	

	//dataset creation properties
	class DProps : public ID {
	protected:
		DProps(hid_t id) : ID(id) {}
	public:
		static const DProps DEFAULT;
		DProps() : ID(H5Pcreate(H5P_DATASET_CREATE)) {}
		DProps(const DProps &dp) : ID(H5Pcopy(dp)) {}
		DProps(DProps &&dp) : ID(std::move(dp)) {}
		virtual ~DProps() {
			if(id) close();
		}
		virtual void close() {
			H5Pclose(id); id = 0;
		}
		//chainable property setters:
		//eg. DProps().chunked().deflate(3).shuffle();
		DProps& compact() {
			H5Pset_layout(id,H5D_COMPACT);
			return *this;
		}
		DProps& contiguous() {
			H5Pset_layout(id,H5D_CONTIGUOUS);
			return *this;
		}
		DProps& chunked() {
			H5Pset_layout(id,H5D_CHUNKED);
			return *this;
		}
		DProps& chunked(const std::vector<hsize_t> &chunk_shape) {
			H5Pset_chunk(id,int(chunk_shape.size()),chunk_shape.data());
			return *this;
		}
		DProps& chunked(const std::vector<hsize_t> &data_shape, size_t item_nbytes, size_t chunk_nbytes=0, size_t line_nbytes = 8192) {
			//if chunk_nbytes is 0, we should compute a value for it:
			if(chunk_nbytes = 0) {
				//get size of dataset in mb
				size_t data_mb = (item_nbytes*std::product(begin(data_shape),end(data_shape),size_t(1))) >> 20;
				data_mb = std::clip<size_t>(data_mb,1,1 << 23); //stay between 1 MB and 8 TB
				//data size -> chunk size: 1 MB -> 1 line, 1 TB -> 1k lines
				//                         2^0  -> 2^0,   2^20 -> 2^10
				//  chunk_size = 2^(log2(data_mb)*10/20)*line_size = sqrt(data_mb)*line_size
				chunk_nbytes = size_t(sqrt(data_mb))*line_nbytes;
			}
			//given chunk_nbytes, how many items should be in the chunk?
			size_t desired_chunk_size(chunk_nbytes / item_nbytes);
			//start with the chunk size the same as the whole shape:
			std::vector<hsize_t> chunk_shape(data_shape);
			//it's possible the shape is <= the desired chunk size
			size_t chunk_size = std::product(begin(chunk_shape),end(chunk_shape),size_t(1));
			if(chunk_size <= desired_chunk_size)
				return chunked(chunk_shape);
			//if it's not, trim each dimension in turn to reduce the chunk size
			for(size_t trim_dim = 0; trim_dim < chunk_shape.size(); ++trim_dim) {
				chunk_shape[trim_dim] = 1; //trim
				//is the new size less than the desired size?
				chunk_size = std::product(begin(chunk_shape),end(chunk_shape),size_t(1));
				if(chunk_size < desired_chunk_size) {
					//yes -- expand it as much as possible, then return
					chunk_shape[trim_dim] = desired_chunk_size / chunk_size;
					break;
				}
			}
			//the minimum chunk shape is a 1 in every dimension
			return chunked(chunk_shape);
		}
		DProps& deflate(unsigned int level) {
			H5Pset_deflate(id,level);
			return *this;
		}
		DProps& fletcher32() {
			H5Pset_fletcher32(id);
			return *this;
		}
		DProps& shuffle() {
			H5Pset_shuffle(id);
			return *this;
		}
		bool is_chunked() const {
			return H5Pget_layout(id) == H5D_CHUNKED;
		}
		std::vector<hsize_t> chunk() const {
			int n = H5Pget_chunk(id,0,nullptr);
			if(n <= 0)
				return std::vector<hsize_t>();
			std::vector<hsize_t> dims(n);
			H5Pget_chunk(id,n,dims.data());
			return dims;
		}
	};

	const DProps DProps::DEFAULT(H5P_DATASET_CREATE_DEFAULT);

	class DSpace : public ID {
		friend class Dataset;
	protected:
		
		DSpace(hid_t id) : ID(id) {}
	public:
		DSpace() : ID(H5Screate(H5S_SIMPLE)) {}
		DSpace(const DSpace &ds) : ID(H5Scopy(ds)) {}
		DSpace(DSpace &&ds) : ID(std::move(ds)) {}
		DSpace(const std::vector<hsize_t>& shape) : ID(0) {
			if(shape.size()) {
				id = H5Screate(H5S_SIMPLE);
				extent(shape,shape);
			} else {
				id = H5Screate(H5S_SCALAR);
			}
		}
		DSpace(const std::vector<hsize_t> &shape, const std::vector<hsize_t> &maxshape) : ID(H5Screate(H5S_SIMPLE)) {
			if(shape.size()) {
				id = H5Screate(H5S_SIMPLE);
				extent(shape,maxshape);
			} else {
				id = H5Screate(H5S_SCALAR);
			}
		}
		~DSpace() {
			if(id) close();
		}
		void close() {
			H5Sclose(id);
		}
		void extent(const std::vector<hsize_t> &shape) {
			extent(shape,shape);
		}
		void extent(const std::vector<hsize_t> &shape, const std::vector<hsize_t> &maxshape) {
			H5Sset_extent_simple(id,int(shape.size()),shape.data(),maxshape.data());	
		}
		std::vector<hsize_t> extent() const {
			if(H5Sget_simple_extent_type(id) == H5S_SIMPLE) {
				int n = H5Sget_simple_extent_ndims(id);
				std::vector<hsize_t> shape(n);
				H5Sget_simple_extent_dims(id,shape.data(),nullptr);
				return shape;
			} else {
				return std::vector<hsize_t>();
			}
		}
		std::vector<hsize_t> max_extent() const {
			if(H5Sget_simple_extent_type(id) == H5S_SIMPLE) {
				int n = H5Sget_simple_extent_ndims(id);
				std::vector<hsize_t> shape(n);
				H5Sget_simple_extent_dims(id,nullptr,shape.data());
				return shape;
			} else {
				return std::vector<hsize_t>();
			}
		}
		std::pair<std::vector<hsize_t>, std::vector<hsize_t>> extents() const {
			if(H5Sget_simple_extent_type(id) == H5S_SIMPLE) {
				int n = H5Sget_simple_extent_ndims(id);
				std::vector<hsize_t> shape(n),maxshape(n);
				H5Sget_simple_extent_dims(id,shape.data(),maxshape.data());
				return make_pair(shape,maxshape);
			} else {
				return make_pair(std::vector<hsize_t>(),std::vector<hsize_t>());
			}
		}
		DSpace& select_all() {
			H5Sselect_all(id);
			return *this;
		}
		DSpace& select_none() {
			H5Sselect_none(id);
			return *this;
		}
		DSpace& select(const std::vector<hsize_t> &start, const std::vector<hsize_t> &count) {
			H5Sselect_hyperslab(id,H5S_SELECT_SET,start.data(),nullptr,count.data(),nullptr);
			return *this;
		}
		static const DSpace SCALAR;
		static const DSpace ALL;
	};

	const DSpace DSpace::SCALAR(H5Screate(H5S_SCALAR));
	const DSpace DSpace::ALL(H5S_ALL); //this is id = 0, so we won't have destructor problems

	//Location
	//Files, Groups, Datasets
	class Location : public ID {
	protected:
		Location() : ID(0) {}
		Location(hid_t _id) : ID(_id) {}
	public:
		Location(Location &&loc) : ID(std::move(loc)) {}
		virtual ~Location() {}
	};

	class Dataset : public Location {
		friend class Group;
	protected:
		DSpace mSpace;
		Dataset() : Location(), mSpace() {}
		Dataset(hid_t id) : Location(id), mSpace(H5Dget_space(id)) {}
	public:
		Dataset(Dataset &&dset) : Location(std::move(dset)) {}
		virtual ~Dataset() {
			if(id) close();
		}
		virtual void close() {
			H5Dclose(id); id = 0;
		}
		DSpace& space() {
			return mSpace;
		}
		//write
		void write(const void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const DSpace& selection = DSpace::ALL) {
			H5Dwrite(id,buffer_type,buffer_shape,selection,H5P_DATASET_XFER_DEFAULT,buffer);
		}
		void write(const void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			write(buffer,buffer_type,buffer_shape,DSpace().select(offset,buffer_shape.extent()));
		}
		template<typename data_t>
		void write(const data_t& buffer, const DSpace& buffer_shape, const DSpace& selection = DSpace::ALL) {
			write(data(buffer),dtype(buffer),buffer_shape);
		}
		template<typename data_t>
		void write(const data_t& buffer, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			write(data(buffer),dtype(buffer),buffer_shape,offset);
		}
		template<typename data_t>
		void write(const data_t& buffer) {
			write(data(buffer),dtype(buffer),shape(buffer));
		}
		template<typename data_t>
		void write(const data_t& buffer,const std::vector<hsize_t>& offset) {
			write(data(buffer),dtype(buffer),shape(buffer),offset);
		}
		//append
		void append(void *buffer, const DType& buffer_type, const DSpace& buffer_shape) {
			//get the current and maximum extents of our space
			std::vector<hsize_t> current, max;
			std::tie(current, max) = mSpace.extents();
			//prepend 1s onto the front of buffer_shape until its the same size as current
			std::vector<hsize_t> bs_old = buffer_shape.extent();
			std::vector<hsize_t> bs_new(current.size(),1);
			std::copy_backward(bs_old.begin(),bs_old.begin(),bs_new.end());
			//extend the file space along the fastest varying dimension that is not yet maximal and is 1 in the buffer shape

		}
		//read - pre-allocated buffer
		void read(void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const DSpace& selection = DSpace::ALL) {
			H5Dread(id,buffer_type,buffer_shape,selection,H5P_DATASET_XFER_DEFAULT,buffer);
		}
		void read(void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			read(buffer,buffer_type,buffer_shape,DSpace().select(offset,buffer_shape.extent()));
		}
		
	};

	//Files, Groups
	class Group : public Location {
	protected:
		Group() : Location() {}
		Group(hid_t id) : Location(id) {}
	public:
		Group(Group &&grp) : Location(std::move(grp)) {}
		virtual ~Group() {
			if(id) close();
		}
		virtual void close() {
			H5Gclose(id);
		}
		Group openGroup(const std::string &name) {
			return Group(H5Gopen(id,name.c_str(),H5P_DEFAULT));
		}
		Group createGroup(const std::string &name) {
			return Group(H5Gcreate(id,name.c_str(),H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT));
		}
		Dataset openDataset(const std::string &name) {
			return Dataset(H5Dopen(id,name.c_str(),H5P_DEFAULT));
		}
		Dataset createDataset(const std::string &name, const DType &dt, const DSpace &space, const DProps& props = DProps::DEFAULT) {
			//if props is chunked, but does not have chunk dimensions yet, we need to compute them
			if(props.is_chunked() && props.chunk().size() == 0) {
				DProps new_props(props);
				new_props.chunked(space.extent(),dt.size());
				return Dataset(H5Dcreate(id,name.c_str(),dt,space,H5P_DEFAULT,new_props,H5P_DEFAULT));
			} else {
				//no changes necessary
				return Dataset(H5Dcreate(id,name.c_str(),dt,space,H5P_DEFAULT,props,H5P_DEFAULT));
			}
		}
		Dataset writeDataset(const std::string &name, const void* buffer, const DType &dt, const DSpace &space, const DProps& props = DProps::DEFAULT) {
			//create and write in one fell swoop
			Dataset dset = createDataset(name,dt,space,props);
			dset.write(buffer,dt,space);
			return dset;
		}
		template<typename data_t>
		Dataset writeDataset(const std::string &name, const data_t& buffer, const DSpace &space, const DProps &props = DProps::DEFAULT) {
			return writeDataset(name,data(buffer),dtype(buffer),space,props);
		}
		template<typename data_t>
		Dataset writeDataset(const std::string &name, const data_t& buffer, const DProps &props = DProps::DEFAULT) {
			return writeDataset(name,data(buffer),dtype(buffer),shape(buffer),props);
		}
		/*
		void createHardLink();
		void createLink();
		*/
	};

	//Files
	class File : public Group {
	public:
		virtual ~File() {
			if(id) close();
		}
		virtual void close() {
			H5Fclose(id);
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
	static allocate_return allocate(const std::vector<hsize_t>&);
};
*/

namespace H5TL {

	//arithmetic type adapter
	template<typename data_t, typename std::enable_if<std::is_arithmetic<data_t>::value>::type>
	struct adapt {
		typedef const DType& dtype_return;
		typedef data_t* data_return;
		typedef const data_t* const_data_return;
		typedef data_t allocate_return;

		static size_t rank(const data_t&) {
			return 0;
		}
		static std::vector<hsize_t> shape(const data_t&) {
			return std::vector<hsize_t>();
		}
		static dtype_return dtype(const data_t&) {
			return dtype<data_t>();
		}
		static data_return data(data_t& d) {
			return &d;
		}
		static const_data_return data(const data_t& d) {
			return &d;
		}
		static allocate_return allocate(const std::vector<hsize_t>&) {
			size_t n = std::product(shape.begin(),shape.end(),hsize_t(1));
			if(n != 1)
				throw runtime_error("Cannot allocate data_t with shape = {"+std::join(", ", begin(shape), end(shape))+"}"); //TODO: convert the shape to a string
			return data_t(0);
		}
	};

	//constant sized array adapter
	template<typename T, size_t N>
	struct adapt<T[N]> {
		typedef const DType& dtype_return;
		typedef T* data_return;
		typedef const T* const_data_return;
		typedef T* allocate_return;

		static size_t rank(const T(&)[N]) {
			return 1;
		}
		static std::vector<hsize_t> shape(const T(&)[N]) {
			return std::vector<hsize_t>(1,N);
		}
		static dtype_return dtype(const T(&)[N]) {
			return dtype<T>();
		}
		static data_return data(T(&d)[N]) {
			return std::begin(d);
		}
		static const_data_return data(const T(&d)[N]) {
			return std::begin(d);
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape) {
			if(std::product(begin(shape),end(shape),hsize_t(1)) != N)
				throw runtime_error("Cannot allocate fixed sized array with conflicting shape = {" + std::join(", ",begin(shape),end(shape)) + "}");
			return new T[N];
		}
	};
	
	//pointer adapter
	template<typename ptr_t, typename std::enable_if<std::is_pointer<ptr_t>::value>::type>
	struct adapt {
		typedef typename std::remove_pointer<ptr_t>::type data_t;
		typedef const DType& dtype_return;
		typedef ptr_t data_return;
		typedef const ptr_t const_data_return;
		typedef ptr_t allocate_return;

		static size_t rank(const ptr_t&) {
			static_assert(false, "Cannot determine rank of data from pointer");
		}
		static std::vector<hsize_t> shape(const ptr_t&) {
			static_assert(false, "Cannot determine shape of data from pointer");
		}
		static dtype_return dtype(const ptr_t&) {
			return dtype<data_t>();
		}
		static data_return data(ptr_t& p) {
			return p;
		}
		static const_data_return data(const ptr_t& p) {
			return p;
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape) {
			size_t n = std::product(shape.begin(),shape.end(),hsize_t(1));
			return new data_t[n];
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
			return DType(H5TL::dtype<std::string>(), s.size());
		}
		static data_return data(std::string& s) {
			return &(s[0]); //s.data() always returns const pointer :(
		}
		static const_data_return data(const std::string& s) {
			return s.data();
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape) {
			if(shape.size() > 1 && std::product(shape.begin()+1,shape.end(),hsize_t(1)) > 1) //TODO: does this make sense?
				throw runtime_error("Cannot allocate std::string with rank > 1");
			return std::string(shape[0],'\0');
		}
	};


	//TODO: complete these changes

	//std::array
	template<typename T, size_t N>
	size_t rank(const std::array<T,N>&) {
		return 1;
	}
	template<typename T, size_t N>
	std::vector<hsize_t> shape(const std::array<T,N>&) {
		return std::vector<hsize_t>(1,N);
	}
	template<typename T, size_t N>
	typename dtype_return<T>::type dtype(const std::array<T,N>&) {
		return dtype<T>();
	}
	template<typename T, size_t N>
	T* data(std::array<T,N>& a) {
		return a.data();
	}
	template<typename T, size_t N>
	const T* data(const std::array<T,N>& a) {
		return a.data();
	}
	template<typename T, size_t N>
	typename allocate_return<std::array<T,N>> allocate<std::array<T,N>>(const std::vector<hsize_t>& shape) {
		return std::array<T,N>();
	}
	//std::vector
	template<typename T, typename A>
	size_t rank(const std::vector<T,A>& vec) {
		return 1;
	}
	template<typename T, typename A>
	std::vector<hsize_t> shape(const std::vector<T,A>& vec) {
		return std::vector<hsize_t>(1,vec.size());
	}
	template<typename T, typename A>
	typename dtype_return<T>::type dtype(const std::vector<T,A>&) {
		return dtype<T>();
	}
	template<typename T, typename A>
	T* data(std::vector<T,A>& vec) {
		return vec.data();
	}
	template<typename T, typename A>
	const T* data(const std::vector<T,A>& vec) {
		return vec.data();
	}
}
#endif

#define H5TL_BLITZ_ADAPT
#ifdef H5TL_BLITZ_ADAPT
//BLITZ++ shape, rank, dtype, data adapters
#include "blitz/array.h"
namespace H5TL {
	template<typename T, int N>
	size_t rank(const blitz::Array<T,N>&) {
		return N;
	}
	template<typename T, int N>
	std::vector<hsize_t> shape(const blitz::Array<T,N>& a) {
		auto s = a.shape();
		return std::vector<hsize_t>(std::begin(s),std::end(s));
	}
	template<typename T, int N>
	typename dtype_return<T>::type dtype(const blitz::Array<T,N>&) {
		return dtype<T>();
	}
	template<typename T, int N>
	T* data(blitz::Array<T,N>& a) {
		return a.data();
	}
	template<typename T, int N>
	const T* data(const blitz::Array<T,N>& a) {
		return a.data();
	}
}
#endif

#define H5TL_OCV_ADAPT
#ifdef H5TL_OCV_ADAPT
//OpenCV shape, rank, dtype, data adapters
#include "opencv2/opencv.hpp"

namespace H5TL {
	size_t rank(const cv::Mat& m) {
		return m.dims + (m.channels() > 1 ? 1 : 0);
	}
	std::vector<hsize_t> shape(const cv::Mat& m) {
		std::vector<hsize_t> tmp(rank(m));
		std::copy(m.size.p, m.size.p + m.dims, tmp.begin());
		if(m.channels() > 1)
			tmp[m.dims] = m.channels();
		return tmp;
	}

	dtype_return<void>::type dtype(const cv::Mat& m) {
		switch(CV_MAT_DEPTH(m.type())) {
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
			throw std::runtime_error("Datatype is not convertible from cv::Mat to H5TL::Dtype");
		}
	}

	template<typename T>
	typename dtype_return<T>::type dtype(const cv::Mat_<T>&) {
		return dtype<T>();
	}

	template<typename T, int N>
	typename dtype_return<T>::type dtype(const cv::Mat_<cv::Vec<T,N>>&) {
		return dtype<T>();
	}

	void* data(cv::Mat& m) {
		return m.data;
	}

	const void* data(const cv::Mat &m) {
		return m.data;
	}
}
#endif
