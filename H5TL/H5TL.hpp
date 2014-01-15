#pragma once
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

	class h5tl_error : public std::runtime_error {
	public:
		explicit h5tl_error(const std::string& what_arg) : std::runtime_error(what_arg) {}
		explicit h5tl_error(const char* what_arg) : std::runtime_error(what_arg) {}
	};

	//Error handling:
	class ErrorHandler {
		//use constructor of private static member to run this when the library loads:
		ErrorHandler() {
			//disable automatic error printing
			H5Eset_auto(H5E_DEFAULT,nullptr,nullptr);
		}
		static const ErrorHandler EH;
	protected:
		static std::string class_name(hid_t class_id) {
			ssize_t len = H5Eget_class_name(class_id,nullptr,0);
			std::string tmp(len,'\0');
			H5Eget_class_name(class_id,&(tmp[0]),len);
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
		static std::string get_error() {
			std::ostringstream oss;
			H5Ewalk2(H5E_DEFAULT,H5E_WALK_DOWNWARD,append_error,&oss);
			H5Eclear2(H5E_DEFAULT);
			return oss.str();
		}
	};
	const ErrorHandler ErrorHandler::EH;

	inline void check(herr_t e) {
		if(e < 0) throw h5tl_error(ErrorHandler::get_error());
	}
	inline bool check_tri(htri_t t) {
		if(t < 0) throw h5tl_error(ErrorHandler::get_error());
		else return t > 0;
	}
	inline hid_t check_id(hid_t id) {
		if(id < 0) throw h5tl_error(ErrorHandler::get_error());
		else return id;
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
	
	class ID {
	protected:
		hid_t id;
		ID() : id(0) {}
		ID(hid_t _id) : id(check_id(_id)) {}
	public:
		ID(const ID& i) : id(i.id) {}
		ID(ID &&i) : id(i.id) {i.id=0;}
		virtual ~ID() {}
		virtual void close()=0;
		virtual operator hid_t() const {return id;}
		virtual bool operator==(const ID& other) const {
			return this == &other || id == other.id;
		}
		virtual bool valid() const {return check_tri(H5Iis_valid(id));}
		virtual operator bool() const {return valid();}

	};

	class DType;
	class PDType;

	class DType : public ID {
		friend class Dataset;
	protected:
		DType(hid_t id) : ID(id) {}
		DType(hid_t id, size_t sz) : ID(H5Tcopy(id)) {
			size(sz);
		}
	public:
		DType(const DType &dt) : ID(check_id(H5Tcopy(dt))) {}
		DType(DType &&dt) : ID(std::move(dt)) {}
		DType(const DType &dt, size_t sz) : ID(check_id(H5Tcopy(dt))) {
			size(sz);
		}
		virtual ~DType() {
			if(id) close();
		}
		void close() {
			check(H5Tclose(id)); id = 0;
		}
		void size(size_t sz) {
			if(sz == 0) check(H5Tset_size(id,H5T_VARIABLE));
			else check(H5Tset_size(id,sz));
		}
		virtual size_t size() const {
			return H5Tget_size(id);
		}
		virtual bool operator==(const DType& other) const {
			return check_tri(H5Tequal(id,other));
		}
	static const PDType NONE;
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
	static const PDType STRING;
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
	const PDType DType::NONE(0);
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
	const PDType DType::STRING(H5T_C_S1);

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
	template<> const DType& dtype<const char*>() {return DType::STRING;}

	//H5TL adapter traits, enable parameter is for using enable_if for specializations
	//specializations for compatible types are at the end of this file
	template<typename data_t, typename enable=void>
	struct adapt {
		//here are required typedefs and member functions for complete compatibility
		typedef typename std::remove_cv<data_t>::type data_t;
		typedef const DType& dtype_return;
		typedef void* data_return;
		typedef const void* const_data_return;
		typedef data_t allocate_return;
		//default implementation will fail on every call
		static size_t rank(const data_t&) {
			static_assert(false,"rank(const data_t&) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
		}
		static std::vector<hsize_t> shape(const data_t&) {
			static_assert(false,"shape(const data_t&) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
		}
		static dtype_return dtype(const data_t&) {
			static_assert(false,"dtype(const data_t&) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
		}
		static data_return data(data_t&) {
			static_assert(false,"data(data_t&) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
		}
		static const_data_return data(const data_t&) {
			static_assert(false,"data(const data_t&) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
		}
		static allocate_return allocate(const std::vector<hsize_t>&, const DType&) {
			static_assert(false,"allocate<data_t>(...) is not supported for data_t. See further error messages for details. Enable, include, or write an appropriate specialization of H5TL::adapt<data_t>.");
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
		Props(hid_t class_id,int) : ID(H5Pcreate(class_id)) {}
	public:
		Props(const Props &p) : ID(H5Pcopy(p)) {}
		Props(Props &&p) : ID(std::move(p)) {}
		virtual ~Props() {
			if(id) close();
		}
		virtual void close() {
			check(H5Pclose(id)); id = 0;
		}
	};

	//link creation properties
	class LProps : public Props {
		LProps(hid_t id) : Props(id) {}
	public:
		static const LProps DEFAULT;
		LProps() : Props(H5P_LINK_CREATE,0) {}
		LProps(const LProps &lp) : Props(lp) {}
		LProps(LProps &&lp) : Props(std::move(lp)) {}
		virtual ~LProps() {}
		LProps& create_intermediate() {
			check(H5Pset_create_intermediate_group(id,1));
			return *this;
		}
	};

	const LProps LProps::DEFAULT = LProps().create_intermediate();

	//dataset creation properties
	class DProps : public Props {
		DProps(hid_t id) : Props(id) {}
	public:
		static const DProps DEFAULT;
		DProps() : Props(H5P_DATASET_CREATE,0) {}
		DProps(const DProps &dp) : Props(dp) {}
		DProps(DProps &&dp) : Props(std::move(dp)) {}
		virtual ~DProps() {
			if(id) close();
		}
		virtual void close() {
			H5Pclose(id); id = 0;
		}
		//chainable property setters:
		//eg. DProps().chunked().deflate(3).shuffle();
		DProps& compact() {
			check(H5Pset_layout(id,H5D_COMPACT));
			return *this;
		}
		DProps& contiguous() {
			check(H5Pset_layout(id,H5D_CONTIGUOUS));
			return *this;
		}
		DProps& chunked() {
			check(H5Pset_layout(id,H5D_CHUNKED));
			return *this;
		}
		DProps& chunked(const std::vector<hsize_t> &chunk_shape) {
			check(H5Pset_chunk(id,int(chunk_shape.size()),chunk_shape.data()));
			return *this;
		}
		DProps& chunked(const std::vector<hsize_t> &data_shape, size_t item_nbytes, size_t chunk_nbytes=0, size_t line_nbytes = 8192) {
			//if chunk_nbytes is 0, we should compute a value for it:
			if(chunk_nbytes = 0) {
				//get size of dataset in mb
				size_t data_mb = (item_nbytes*std::product(data_shape.begin(),data_shape.end(),size_t(1))) >> 20;
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
			size_t chunk_size = std::product(chunk_shape.begin(),chunk_shape.end(),size_t(1));
			if(chunk_size <= desired_chunk_size)
				return chunked(chunk_shape);
			//if it's not, trim each dimension in turn to reduce the chunk size
			for(size_t trim_dim = 0; trim_dim < chunk_shape.size(); ++trim_dim) {
				chunk_shape[trim_dim] = 1; //trim
				//is the new size less than the desired size?
				chunk_size = std::product(chunk_shape.begin(),chunk_shape.end(),size_t(1));
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
			check(H5Pset_deflate(id,level));
			return *this;
		}
		DProps& fletcher32() {
			check(H5Pset_fletcher32(id));
			return *this;
		}
		DProps& shuffle() {
			check(H5Pset_shuffle(id));
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

	class Selection : public ID {
	protected:
		Selection(hid_t id) : ID(id) {}
	public:
		Selection() : ID(H5Screate(H5S_SIMPLE)) {}
		Selection(const Selection &s) : ID(H5Scopy(s)) {}
		Selection(Selection &&s) : ID(std::move(s)) {}
		Selection(const std::vector<hsize_t> &start, const std::vector<hsize_t> &count) : ID(H5Screate(H5S_SIMPLE)) {
			set(start,count);
		}
		virtual ~Selection() {
			if(id) close();
		}
		void close() {
			check(H5Sclose(id));
		}
		Selection& all() {
			check(H5Sselect_all(id)); return *this;
		}
		Selection& none() {
			check(H5Sselect_none(id)); return *this;
		}
		Selection& set(const std::vector<hsize_t> &start, const std::vector<hsize_t> &count) {
			check(H5Sselect_hyperslab(id,H5S_SELECT_SET,start.data(),nullptr,count.data(),nullptr));
			return *this;
		}
		static const Selection ALL;  
	};

	const Selection Selection::ALL(H5S_ALL); //this is id = 0, so we won't have destructor problems

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
				id = check_id(H5Screate(H5S_SIMPLE));
				extent(shape,shape);
			} else {
				id = check_id(H5Screate(H5S_SCALAR));
			}
		}
		DSpace(const std::vector<hsize_t> &shape, const std::vector<hsize_t> &maxshape) : ID(H5Screate(H5S_SIMPLE)) {
			if(shape.size()) {
				id = check_id(H5Screate(H5S_SIMPLE));
				extent(shape,maxshape);
			} else {
				id = check_id(H5Screate(H5S_SCALAR));
			}
		}
		~DSpace() {
			if(id) close();
		}
		void close() {
			check(H5Sclose(id));
		}
		void extent(const std::vector<hsize_t> &shape) {
			extent(shape,shape);
		}
		void extent(const std::vector<hsize_t> &shape, const std::vector<hsize_t> &maxshape) {
			check(H5Sset_extent_simple(id,int(shape.size()),shape.data(),maxshape.data()));
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
		
		static const DSpace SCALAR;
	};

	const DSpace DSpace::SCALAR(H5Screate(H5S_SCALAR));

	//Location
	//Files, Groups, Datasets
	class Location : public ID {
	protected:
		Location() : ID(0) {}
		Location(hid_t _id) : ID(_id) {}
	public:
		Location(Location &&loc) : ID(std::move(loc)) {}
		virtual ~Location() {}
		virtual bool exists() {
			return check_tri(H5Oexists_by_name(id,".",H5P_LINK_ACCESS_DEFAULT));
		}
	};

	class Dataset : public Location {
		friend class Group;
	protected:
		Dataset() : Location() {}
		Dataset(hid_t id) : Location(id) {}
	public:
		Dataset(Dataset &&dset) : Location(std::move(dset)) {}
		virtual ~Dataset() {
			if(id) close();
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
		//Dimension Scale:
		/*
		void makeScale(const std::string &name = std::string()) {
			if(name.size())
				H5DSset_scale(id,name.c_str());
			else
				H5DSset_scale(id,nullptr);
		}
		bool isScale() {
			return H5DSis_scale(id) > 0;
		}
		void attachScale(Dataset& scale, unsigned int dim) {
			H5DSattach_scale(id,scale,dim);
		}
		bool isAttached(const Dataset& scale, unsigned int dim) {
			return H5DSis_attached(id,scale,dim) > 0;
		}
		void detachScale(Dataset& scale, unsigned int dim) {
			H5DSdetach_scale(id,scale,dim);
		}
		*/
		//write
		void write(const void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			check(H5Dwrite(id,buffer_type,buffer_shape,selection,H5P_DATASET_XFER_DEFAULT,buffer));
		}
		void write(const void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			write(buffer,buffer_type,buffer_shape,Selection(offset,buffer_shape.extent()));
		}
		template<typename data_t>
		void write(const data_t& buffer, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			write(H5TL::data(buffer),H5TL::dtype(buffer),buffer_shape);
		}
		template<typename data_t>
		void write(const data_t& buffer, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			write(H5TL::data(buffer),H5TL::dtype(buffer),buffer_shape,offset);
		}
		template<typename data_t>
		void write(const data_t& buffer, const Selection& selection = Selection::ALL) {
			write(H5TL::data(buffer),H5TL::dtype(buffer),H5TL::shape(buffer));
		}
		template<typename data_t>
		void write(const data_t& buffer,const std::vector<hsize_t>& offset) {
			write(H5TL::data(buffer),H5TL::dtype(buffer),H5TL::shape(buffer),offset);
		}
		//append
		void append(void *buffer, const DType& buffer_type, const DSpace& buffer_shape) {
			//get the current and maximum extents of our space
			std::vector<hsize_t> current, max;
			std::tie(current, max) = space().extents();
			//prepend 1s onto the front of buffer_shape until its the same size as current
			std::vector<hsize_t> bs_old = buffer_shape.extent();
			std::vector<hsize_t> bs_new(current.size(),1);
			std::copy_backward(bs_old.begin(),bs_old.begin(),bs_new.end());
			//extend the file space along the fastest varying dimension that is not yet maximal and is 1 in the buffer shape

		}
		//read
		void read(void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			//if buffer_shape is empty, allocate space to hold the selection???
			check(H5Dread(id,buffer_type,buffer_shape,selection,H5P_DATASET_XFER_DEFAULT,buffer));
		}
		void read(void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			read(buffer,buffer_type,buffer_shape,Selection(offset,buffer_shape.extent()));
		}
		template<typename data_t>
		void read(data_t& buffer, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			read(H5TL::data(buffer),H5TL::dtype(buffer),buffer_shape,selection);
		}
		template<typename data_t>
		void read(data_t& buffer, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			read(H5TL::data(buffer),H5TL::dtype(buffer),buffer_shape,offset);
		}
		template<typename data_t>
		void read(data_t& buffer, const Selection& selection = Selection::ALL) {
			read(H5TL::data(buffer),H5TL::dtype(buffer),H5TL::shape(buffer),selection);
		}
		template<typename data_t>
		void read(data_t& buffer, const std::vector<hsize_t>& offset) {
			read(H5TL::data(buffer),H5TL::dtype(buffer),H5TL::shape(buffer),offset);
		}
		//read with allocate
		template<typename data_t>
		typename adapt<data_t>::allocate_return
		read(const std::vector<hsize_t>& offset, const std::vector<hsize_t>& buffer_shape) {
			auto buffer = H5TL::allocate<data_t>(buffer_shape,dtype());
			read(buffer,offset);
			return buffer;
		}
		template<typename data_t>
		typename adapt<data_t>::allocate_return
		read() {
			auto buffer = H5TL::allocate<data_t>(space().extent(),dtype());
			read(buffer);
			return buffer;
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
			check(H5Gclose(id));
		}
		using ID::valid;
		virtual bool valid(const std::string &path) {
			//Copy logic of H5TLpath_valid
			//the path to the current object or root is always valid:
			if(path == "." || path == "/" || path == "./")
				return true;

			//make a working copy of the path
			std::string current_path(path);
			//we want to check each step along the path, except the last
			//so find each path delimiter in turn
			size_t delimiter_pos = 0;
			if(path.compare(0,1,"/") == 0)
				delimiter_pos = 1; //skip the first '/' of an absolute path
			else if(path.compare(0,2,"./") == 0)
				delimiter_pos = 2; //the path starts with "./" -- skip this part
			while((delimiter_pos = path.find('/',delimiter_pos+1)) != std::string::npos) {
				//if the path ends with a '/', we should ignore it in this loop
				if(delimiter_pos == path.size()-1) break;
				//change the character to null
				current_path[delimiter_pos] = '\0';
				//check the link
				if(!check_tri(H5Lexists(id,current_path.c_str(),H5P_LINK_ACCESS_DEFAULT))) return false;
				//check that the link resolves to an object
				if(!check_tri(H5Oexists_by_name(id,current_path.c_str(),H5P_LINK_ACCESS_DEFAULT))) return false;
				//replace the delimiter and move on to the next step of the path
				current_path[delimiter_pos] = '/';
			}
			//all of the steps of the path have been checked, except the last
			//make sure the link to the last object exists
			return check_tri(H5Lexists(id,current_path.c_str(),H5P_LINK_ACCESS_DEFAULT));
		}
		virtual bool exists(const std::string &path) {
			if(!valid(path))
				return false;
			return check_tri(H5Oexists_by_name(id,path.c_str(),H5P_LINK_ACCESS_DEFAULT));
		}
		Group group(const std::string& name) {
			if(exists(name))
				return openGroup(name); //if it's not a group, this will throw an exception
			else
				return createGroup(name);
		}
		Group openGroup(const std::string &name) {
			return Group(H5Gopen(id,name.c_str(),H5P_GROUP_ACCESS_DEFAULT));
		}
		Group createGroup(const std::string &name) {
			return Group(H5Gcreate(id,name.c_str(),LProps::DEFAULT,H5P_GROUP_CREATE_DEFAULT,H5P_GROUP_ACCESS_DEFAULT));
		}
		Dataset openDataset(const std::string &name) {
			return Dataset(H5Dopen(id,name.c_str(),H5P_DATASET_ACCESS_DEFAULT));
		}
		Dataset createDataset(const std::string &name, const DType &dt, const DSpace &space, const DProps& props = DProps::DEFAULT) {
			//if props is chunked, but does not have chunk dimensions yet, we need to compute them
			if(props.is_chunked() && props.chunk().size() == 0) {
				DProps new_props(props);
				new_props.chunked(space.extent(),dt.size());
				return Dataset(H5Dcreate(id,name.c_str(),dt,space,LProps::DEFAULT,new_props,H5P_DATASET_ACCESS_DEFAULT));
			} else {
				//no changes necessary
				return Dataset(H5Dcreate(id,name.c_str(),dt,space,LProps::DEFAULT,props,H5P_DATASET_ACCESS_DEFAULT));
			}
		}
		//create dataset and write data in
		Dataset write(const std::string &name, const void* buffer, const DType &dt, const DSpace &space, const DProps& props = DProps::DEFAULT) {
			//create and write in one fell swoop
			Dataset dset = createDataset(name,dt,space,props);
			dset.write(buffer,dt,space);
			return dset;
		}
		template<typename data_t>
		Dataset write(const std::string &name, const data_t& buffer, const DSpace &space, const DProps &props = DProps::DEFAULT) {
			return write(name,H5TL::data(buffer),H5TL::dtype(buffer),space,props);
		}
		template<typename data_t>
		Dataset write(const std::string &name, const data_t& buffer, const DProps &props = DProps::DEFAULT) {
			return write(name,H5TL::data(buffer),H5TL::dtype(buffer),H5TL::shape(buffer),props);
		}
		//open and read dataset
		Dataset read(const std::string &name, void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			Dataset ds = openDataset(name);
			ds.read(buffer,buffer_type,buffer_shape,selection);
			return ds;
		}
		Dataset read(const std::string &name, void* buffer, const DType& buffer_type, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			return read(name, buffer,buffer_type,buffer_shape,Selection(offset,buffer_shape.extent()));
		}
		template<typename data_t>
		Dataset read(const std::string &name, data_t& buffer, const DSpace& buffer_shape, const Selection& selection = Selection::ALL) {
			return read(name, H5TL::data(buffer),H5TL::dtype(buffer),buffer_shape,selection);
		}
		template<typename data_t>
		Dataset read(const std::string &name, data_t& buffer, const DSpace& buffer_shape, const std::vector<hsize_t>& offset) {
			return read(name, H5TL::data(buffer),H5TL::dtype(buffer),buffer_shape,offset);
		}
		template<typename data_t>
		Dataset read(const std::string &name, data_t& buffer, const Selection& selection = Selection::ALL) {
			return read(name, H5TL::data(buffer),H5TL::dtype(buffer),H5TL::shape(buffer),selection);
		}
		template<typename data_t>
		Dataset read(const std::string &name, data_t& buffer, const std::vector<hsize_t>& offset) {
			return read(name, H5TL::data(buffer),H5TL::dtype(buffer),H5TL::shape(buffer),offset);
		}
		//open dataset, read with allocate
		template<typename data_t>
		typename adapt<data_t>::allocate_return
		read(const std::string &name, const std::vector<hsize_t>& offset, const std::vector<hsize_t>& buffer_shape) {
			Dataset ds = openDataset(name);
			return ds.read<data_t>(offset,buffer_shape);
		}
		template<typename data_t>
		typename adapt<data_t>::allocate_return
		read(const std::string &name) {
			Dataset ds = openDataset(name);
			return ds.read<data_t>();
		}

		void createHardLink(const std::string& name, const Group& target_group, const std::string& target) {
			check(H5Lcreate_hard(target_group,target.c_str(),id,name.c_str(),LProps::DEFAULT,H5P_LINK_ACCESS_DEFAULT));
		}
		void createHardLink(const std::string& name, const std::string& target) {
			createHardLink(name,*this,target);
		}
		void createHardLink(const std::string& name, const Location& target) {
			check(H5Olink(target,id,name.c_str(),LProps::DEFAULT,H5P_LINK_ACCESS_DEFAULT));
		}
		void createLink(const std::string& name, const std::string& target) {
			check(H5Lcreate_soft(path.c_str(),id,name.c_str(),LProps::DEFAULT,H5P_LINK_ACCESS_DEFAULT));
		}
		void createLink(const std::string& name, const std::string& file, const std::string& target) {
			check(H5Lcreate_external(file.c_str(),target.c_str(),id,name.c_str(),LProps::DEFAULT,H5P_LINK_ACCESS_DEFAULT));
		}
	};

	//Files
	class File : public Group {
	public:
		enum OpenMode : unsigned int {
#pragma push_macro("H5CHECK") //compatible with MSVC, gcc, clang
#define H5CHECK //need to make this not call H5check() so that the following expressions are constant:
			TRUNCATE = H5F_ACC_TRUNC,
			CREATE = H5F_ACC_EXCL,
			READ_WRITE = H5F_ACC_RDWR,
			READ = H5F_ACC_RDONLY
#pragma pop_macro("H5CHECK")
		};
		File(const std::string& name, const OpenMode& mode = READ_WRITE) {
			if(mode == TRUNCATE || mode == CREATE) {
				id = check_id(H5Fcreate(name.c_str(),mode,H5P_FILE_CREATE_DEFAULT,H5P_FILE_ACCESS_DEFAULT));
			} else {
				hid_t tmp_id = H5Fopen(name.c_str(),mode,H5P_FILE_ACCESS_DEFAULT);
				if(tmp_id < 0) //if we failed to open, let's try creating the file
					tmp_id = H5Fcreate(name.c_str(),CREATE,H5P_FILE_CREATE_DEFAULT,H5P_FILE_ACCESS_DEFAULT);
				id = check_id(tmp_id);
			}
		}
		virtual ~File() {
			if(id) close();
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

	//arithmetic type adapter
	template<typename number_t>
	struct adapt<number_t,typename std::enable_if<std::is_arithmetic<number_t>::value>::type> {
		typedef typename std::remove_cv<number_t>::type data_t;
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
			return H5TL::dtype<data_t>();
		}
		static data_return data(data_t& d) {
			return &d;
		}
		static const_data_return data(const data_t& d) {
			return &d;
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			size_t n = std::product(shape.begin(),shape.end(),hsize_t(1));
			if(n != 1)
				throw runtime_error("Cannot allocate data_t with shape = {"+std::join(", ", shape.begin(), shape.end())+"}"); //TODO: convert the shape to a string
			return data_t(0);
		}
	};
	 
	//constant sized array adapter
	template<typename T, size_t N>
	struct adapt<T[N]> {
		typedef typename std::remove_cv<T>::type data_t;
		typedef const DType& dtype_return;
		typedef data_t* data_return;
		typedef const data_t* const_data_return;
		typedef data_t* allocate_return;

		static size_t rank(const T(&)[N]) {
			return 1;
		}
		static std::vector<hsize_t> shape(const data_t(&)[N]) {
			return std::vector<hsize_t>(1,N);
		}
		static dtype_return dtype(const data_t(&)[N]) {
			return H5TL::dtype<data_t>();
		}
		static data_return data(data_t(&d)[N]) {
			return std::begin(d);
		}
		static const_data_return data(const data_t(&d)[N]) {
			return std::begin(d);
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			if(std::product(begin(shape),end(shape),hsize_t(1)) != N)
				throw runtime_error("Cannot allocate fixed sized array with conflicting shape = {" + std::join(", ",shape.begin(),shape.end()) + "}");
			return new T[N];
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
			return DType(DType::STRING,N);
		}
		static const_data_return data(const data_t(&d)[N]) {
			return std::begin(d);
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			static_assert(false,"Cannot allocate a C-style string. Please use std::string instead");
		}
	};
	
	//pointer adapter
	template<typename ptr_t>
	struct adapt<ptr_t, typename std::enable_if<std::is_pointer<ptr_t>::value 
												&& !std::is_void<typename std::remove_pointer<ptr_t>::type>::value
										>::type> {
		typedef typename std::remove_cv<typename std::remove_pointer<ptr_t>::type>::type data_t;
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
			return H5TL::dtype<data_t>();
		}
		static data_return data(ptr_t& p) {
			return p;
		}
		static const_data_return data(const ptr_t& p) {
			return p;
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			size_t n = std::product(shape.begin(),shape.end(),hsize_t(1));
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
			size_t n = std::product(shape.begin(),shape.end(),hsize_t(dt.size()));
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
			if(shape.size() > 1 && std::product(shape.begin()+1,shape.end(),hsize_t(1)) > 1) //TODO: does this make sense?
				throw std::runtime_error("Cannot allocate std::string with rank > 1");
			return std::string(shape[0],'\0');
		}
	};

	//std::array
	template<typename T, size_t N>
	struct adapt<std::array<T,N>,void> {
		typedef const DType& dtype_return;
		typedef T* data_return;
		typedef const T* const_data_return;
		typedef std::array<T,N> allocate_return;

		static size_t rank(const std::array<T,N>&) {
			return 1;
		}
		static std::vector<hsize_t> shape(const std::array<T,N>&) {
			return std::vector<hsize_t>(1,hsize_t(N));
		}
		static dtype_return dtype(const std::array<T,N>&) {
			return H5TL::dtype<T>();
		}
		static data_return data(std::array<T,N> &d) {
			return d.data();
		}
		static const_data_return data(const std::array<T,N>& d) {
			return d.data();
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			if(std::product(shape.begin(),shape.end(),hsize_t(1)) != N)
				throw runtime_error("Cannot allocate std::array<T,N> with shape = {" + std::join(", ",shape.begin(),shape.end()) + "}.");
			return std::array<T,N>();
		}
	};
	
	//std::vector
	template<typename T>
	struct adapt<std::vector<T>> {
		
		typedef const DType& dtype_return;
		typedef typename std::vector<T>::pointer data_return;
		typedef typename std::vector<T>::const_pointer const_data_return;
		typedef std::vector<T> allocate_return;

		static size_t rank(const std::vector<T>&) {
			return 1;
		}
		static std::vector<hsize_t> shape(const std::vector<T>& v) {
			return std::vector<hsize_t>(1,v.size());
		}
		static dtype_return dtype(const std::vector<T>&) {
			return H5TL::dtype<T>();
		}
		static data_return data(std::vector<T>& v) {
			return v.data();
		}
		static const_data_return data(const std::vector<T>& v) {
			return v.data();
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			return std::vector<T>(std::product(shape.begin(),shape.end(),hsize_t(1)),T());
		}
	};
}
#endif

#define H5TL_BLITZ_ADAPT
#ifdef H5TL_BLITZ_ADAPT
//BLITZ++ shape, rank, dtype, data adapters
#include "blitz/array.h"
namespace H5TL {
	template<typename T, int N>
	struct adapt<blitz::Array<T,N>> {
		typedef const DType& dtype_return;
		
		typedef T* data_return;
		typedef const T* const_data_return;
		typedef blitz::Array<T,N> allocate_return;

		static size_t rank(const blitz::Array<T,N>&) {
			return N;
		}
		static std::vector<hsize_t> shape(const blitz::Array<T,N>& d) {
			const blitz::TinyVector<int,N> &s = d.shape();
			return std::vector<hsize_t>(s.begin(),s.end());
		}
		static dtype_return dtype(const blitz::Array<T,N>&) {
			return H5TL::dtype<T>();
		}
		static data_return data(blitz::Array<T,N>& d) {
			return d.data();
		}
		static const_data_return data(const blitz::Array<T,N>& d) {
			return d.data();
		}
		static allocate_return allocate(const std::vector<hsize_t>& shape, const DType&) {
			if(shape.size() > N)
				throw runtime_error("Cannot allocate blitz::Array<T,N> with higher dimensionality shape = {" + std::join(", ",shape.begin(),shape.end()) + "}.");
			blitz::TinyVector<int,N> extent(1);
			//we can't use std::copy_backward on MSVC because blitz::TinyVector::iterator is just a raw pointer
			auto in = shape.end();
			auto out = extent.end();
			while(in != shape.begin())
				*(--out) = int(*(--in));
			return blitz::Array<T,N>(extent);
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
			switch(CV_MAT_DEPTH(cv_type)) {
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
			if(dt == DType::UINT8)
				return CV_8U;
			else if(dt == DType::INT8)
				return CV_8S;
			else if(dt == DType::UINT16)
				return CV_16U;
			else if(dt == DType::INT16)
				return CV_16S;
			else if(dt == DType::INT32)
				return CV_32S;
			else if(dt == DType::FLOAT)
				return CV_32F;
			else if(dt == DType::DOUBLE)
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
			if(d.channels() > 1)
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
			std::transform(shape.begin(),shape.end(),sz.begin(),[](hsize_t x){return int(x);});
			return cv::Mat(int(sz.size()),sz.data(),h5_cv(dt));
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
