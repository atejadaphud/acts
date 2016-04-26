///////////////////////////////////////////////////////////////////
// ExtrapolationEngine.h, ACTS project
///////////////////////////////////////////////////////////////////

#ifndef ACTS_EXTRAPOLATIONENGINE_EXTRAPOLATIONENGINE_H
#define ACTS_EXTRAPOLATIONENGINE_EXTRAPOLATIONENGINE_H 1

#include "ACTS/Extrapolation/ExtrapolationCell.h"
#include "ACTS/Extrapolation/detail/ExtrapolationMacros.h"
#include "ACTS/Extrapolation/IExtrapolationEngine.h"
#include "ACTS/Extrapolation/IPropagationEngine.h"
#include "ACTS/Extrapolation/INavigationEngine.h"
#include "ACTS/EventData/TrackParameters.h"
#include "ACTS/EventData/NeutralParameters.h"

namespace Acts {
  
    class TrackingGeometry;  
    class Surface;
    class BoundaryCheck;     
    
    /** @class ExtrapolationEngine 
        
        Master extrapolation engine for extrapolation through the TrackingGeometry,
        it delegates the extrapolation to optimised engines, handing over the ExtrapolationCell
        as internal cache.
    
        There are identical interfaces for charged and neutral track parameters.
        Providing a destination surface is optional, if no destination surface is given the extrapolation 
        process can be stopped by other directives, e.g. stopping at a certain path limit, material limit
        or with a change of detector signature.
       
        @author Andreas.Salzburger -at- cern.ch 
    */
    class ExtrapolationEngine : virtual public IExtrapolationEngine {
    
        /** @struct Config
        
            Configuration struct to be used for this ExtrapolationEngine.
            This has to be prepared by the framework/main and can be either given to the ExtrapolationEngine at construction or
            with the setConfig method.
        */
        struct Config {
            
            std::shared_ptr<const TrackingGeometry>               trackingGeometry;      //!< the tracking geometry used by the navigator
            std::vector< std::shared_ptr<IExtrapolationEngine> >  extrapolationEngines;  //!< the extrapolation engines for different geometry layouts
            std::shared_ptr<IPropagationEngine>                   propagationEngine;     //!< the used propagation engine for navigation initialization
            std::shared_ptr<INavigationEngine>                    navigationEngine;      //!< access to tracking geometry (unique?)
            std::string                                           prefix;                //!< output prefix
            std::string                                           postfix;               //!< output postfix
            
            Config() :
              trackingGeometry(nullptr),
              extrapolationEngines(),
              propagationEngine(nullptr),
              navigationEngine(nullptr),
              prefix("[ME] - "),
              postfix(" - ")
            {}             
            
        };    
          
        public:
            /** Constructor */
            ExtrapolationEngine(const Config& eeConfig);
            
            /** Destructor */
            ~ExtrapolationEngine();
            
            using IExtrapolationEngine::extrapolate;
            
            /** charged extrapolation - public interface */
            ExtrapolationCode extrapolate(ExCellCharged& ecCharged,
                                          const Surface* sf = nullptr,
                                          const BoundaryCheck& bcheck = true) const final;
            
            /** neutral extrapolation - public interface */
            ExtrapolationCode extrapolate(ExCellNeutral& ecNeutral,
                                          const Surface* sf = nullptr,
                                          const BoundaryCheck& bcheck = true) const final;
                             
                             
            /** define for which GeometrySignature this extrapolator is valid - this is GLOBAL */
            GeometryType geometryType() const final;                           
                               
            /** Set configuration method */
            void setConfiguration(const Config& eeConfig);
      
            /** Get configuration method */
            Config getConfiguration() const;                                 
                               
        protected:
            /** ExtrapolationEngine config object */
            Config m_config;
             
        private:
            /** main extrapolation method, templated to chared/neutral */
            template <class T> ExtrapolationCode extrapolateT(ExtrapolationCell<T>& eCell,
                                                              const Surface* sf = nullptr,
                                                              PropDirection dir=alongMomentum,
                                                              const BoundaryCheck& bcheck = true) const;
                
            /** initialization of mavigation method */                                      
            template <class T>  ExtrapolationCode initNavigation(ExtrapolationCell<T>& eCell,
                                                                 const Surface* sf = nullptr,
                                                                 PropDirection dir=alongMomentum) const;
    };
    
    /** Return the geometry type, it's the master */
    inline GeometryType  ExtrapolationEngine::geometryType() const  { return Acts::Master; }
    
    /** Return the configuration object */    
    inline ExtrapolationEngine::Config ExtrapolationEngine::getConfiguration() const { return m_config; }

} // end of namespace

//!< define the templated function    
#include "ACTS/Extrapolation/detail/ExtrapolationEngine.icc"  

#endif // ACTS_EXTRAPOLATIONENGINE_EXTRAPOLATIONENGINE_H 

